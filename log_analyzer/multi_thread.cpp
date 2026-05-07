#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
#include <unordered_map>
#include <iomanip>
#include <chrono>
#include <ctime>
#include <thread>
#include <future>
#include <stdexcept>

struct LogRecord {
    std::string timestamp;
    std::string level;
    std::string tag;
    std::string message;
};

struct ErrorEvent {
    std::string timestamp;
    std::string level;
    std::string message;
};

struct AnalysisResult {
    size_t totalLines = 0;
    size_t parsedLines = 0;
    size_t ignoredLines = 0;

    std::unordered_map<std::string, size_t> levelCount;
    std::unordered_map<std::string, size_t> eventCount;
    std::unordered_map<std::string, size_t> errorEventCount;
    std::vector<std::streampos> errorOffsets; 

    std::vector<ErrorEvent> errorEvents;
};

std::string convertLevel(const char& levelChar) {
    switch (levelChar) {
        case 'V': return "VERBOSE";
        case 'D': return "DEBUG";
        case 'I': return "INFO";
        case 'W': return "WARN";
        case 'E': return "ERROR";
        default: return "UNKNOWN";
    }
}

bool isErrorLevel(const std::string& level) {
    return level == "ERROR";
}

std::string trim(const std::string& str) {
    size_t start = str.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    size_t end = str.find_last_not_of(" \t\r\n");
    return str.substr(start, end - start + 1);
}

std::string getCurrentTimeStr() {
    auto now = std::chrono::system_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);
    struct tm* parts = std::localtime(&now_c);

    char buffer[64];
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d_%H-%M-%S", parts);
    return std::string(buffer);
}

bool parseLogLine(const std::string& line, LogRecord& outRecord) {
    if (line.empty()) return false;

    size_t pos = 0;
    size_t endPos = 0;

    endPos = line.find(' ', pos);
    if (endPos == std::string::npos) return false;
    std::string date = line.substr(pos, endPos - pos);
    
    pos = line.find_first_not_of(' ', endPos);
    if (pos == std::string::npos) return false;

    endPos = line.find(' ', pos);
    if (endPos == std::string::npos) return false;
    std::string time = line.substr(pos, endPos - pos);

    for (int i = 0; i < 2; ++i) {
        pos = line.find_first_not_of(' ', endPos);
        if (pos == std::string::npos) return false;
        endPos = line.find(' ', pos);
        if (endPos == std::string::npos) return false;
    }

    pos = line.find_first_not_of(' ', endPos);
    if (pos == std::string::npos) return false;
    char levelChar = line[pos];
    
    pos = line.find_first_not_of(' ', pos + 1);
    if (pos == std::string::npos) return false;

    size_t colonPos = line.find(": ", pos);
    if (colonPos == std::string::npos) return false;

    std::string tag = trim(line.substr(pos, colonPos - pos));

    size_t msgStart = colonPos + 2;
    if (msgStart < line.size()) {
        msgStart = line.find_first_not_of(' ', msgStart); 
    }
    
    std::string message = "";
    if (msgStart != std::string::npos && msgStart < line.size()) {
        message = line.substr(msgStart);
    }

    outRecord.timestamp = date + " " + time;
    outRecord.level = convertLevel(levelChar);
    outRecord.tag = tag;
    outRecord.message = message;

    return true;
}

void analyzeRecord(const LogRecord& record, std::streampos offset, AnalysisResult& result) {
    result.parsedLines++;

    result.levelCount[record.level]++;
    result.eventCount[record.tag]++;

    if (isErrorLevel(record.level)) {
        // ErrorEvent errorEvent;
        // errorEvent.timestamp = record.timestamp;
        // errorEvent.level = record.level;
        // errorEvent.message = record.message;

        result.errorEventCount[record.tag]++;
        result.errorOffsets.push_back(offset); 
    }
}

AnalysisResult analyzeChunk(const std::string& filePath, std::streampos startOffset, std::streampos endOffset) {
    AnalysisResult result;

    std::ifstream file(filePath, std::ios::binary); 
    if (!file.is_open()) return result;
    // 스레드 별로 chunck 먹일 예정
    file.seekg(startOffset);

    if (startOffset > 0) {
        std::string temp;
        std::getline(file, temp); // 시작 위치에서 한 줄 읽어서 버리기
    }

    std::string line;
    while (true) {
        std::streampos currentOffset = file.tellg();
        
        // EOF나 읽을 위치가 끝을 넘어섰으면 종료
        if (currentOffset == std::streampos(-1) || currentOffset >= endOffset) {
            break;
        }

        if (!std::getline(file, line)) {
            break;
        }

        result.totalLines++;
        LogRecord record;
        if(!parseLogLine(line, record)){
            result.ignoredLines++;
            continue;
        }

        analyzeRecord(record, currentOffset, result);
    }

    return result;
}

AnalysisResult mergeResults(const std::vector<AnalysisResult>& results) {
    AnalysisResult global;
    
    for (const auto& r : results) {
        global.totalLines += r.totalLines;
        global.parsedLines += r.parsedLines;
        global.ignoredLines += r.ignoredLines;

        for (const auto& [k, v] : r.levelCount) global.levelCount[k] += v;
        for (const auto& [k, v] : r.eventCount) global.eventCount[k] += v;
        for (const auto& [k, v] : r.errorEventCount) global.errorEventCount[k] += v;

        global.errorOffsets.insert(global.errorOffsets.end(), r.errorOffsets.begin(), r.errorOffsets.end());
    }

    // 스레드별로 작업하여 오프셋 순서가 뒤섞였으므로, 오름차순으로 정렬
    // 순서 정렬이 필요한 이유는 원본 파일에서 에러 이벤트를 읽어올 때 파일을 순차적으로 읽는 것이 디스크 I/O 효율에 더 좋기 때문
    std::sort(global.errorOffsets.begin(), global.errorOffsets.end());

    return global;
}

AnalysisResult analyzeFileMultiThreaded(const std::string& filePath) {
    std::ifstream file(filePath, std::ios::ate | std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file: " + filePath);
    }
    std::streampos fileSize = file.tellg();
    file.close();

    size_t numThreads = std::thread::hardware_concurrency();
    if (numThreads == 0) numThreads = 4;
    
    std::streampos chunkSize = fileSize / numThreads;
    std::vector<std::future<AnalysisResult>> futures;

    for (size_t i = 0; i < numThreads; ++i) {
        std::streampos start = i * chunkSize;
        std::streampos end = (i == numThreads - 1) ? fileSize : start + chunkSize;
        
        futures.push_back(std::async(std::launch::async, analyzeChunk, filePath, start, end));
    }

    std::vector<AnalysisResult> chunkResults;
    for (auto& f : futures) {
        chunkResults.push_back(f.get());
    }

    return mergeResults(chunkResults);
}

std::vector<std::pair<std::string, size_t>> sortByCountDesc(const std::unordered_map<std::string, size_t>& map) {
    std::vector<std::pair<std::string, size_t>> items(map.begin(), map.end());
    std::sort(items.begin(), items.end(), [](const auto& a, const auto& b) {
        return a.second > b.second;
    });
    return items;
}

void saveResultToFile(const std::string& filename, const AnalysisResult& result, const std::string& originalFilePath) {
    std::ofstream outFile(filename);

    if (!outFile.is_open()) {
        std::cerr << "can't open " << filename << std::endl;
        return;
    }

    outFile << "===== Android Log Analysis Result =====\n\n";
    outFile << "Total lines : " << result.totalLines << "\n";
    outFile << "Parsed lines: " << result.parsedLines << "\n";
    outFile << "Ignored lines: " << result.ignoredLines << "\n\n";

    outFile << "Level Count:\n";
    auto sortedLevels = sortByCountDesc(result.levelCount);
    for (const auto& [level, count] : sortedLevels) {
        outFile << level << " : " << count << "\n";
    }

    outFile << "\nError Producers (Tag : Error Count / Total Count):\n\n";
    auto sortedErrorEvents = sortByCountDesc(result.errorEventCount);
    size_t errCount = 0;
    for (const auto& [tag, errors] : sortedErrorEvents) {
        size_t total = result.eventCount.at(tag);
        double errorRate = (double)errors / total * 100.0;
        outFile << tag << " : " << errors << " / " << total << " (" << std::fixed << std::setprecision(2) << errorRate << "%)\n";
        if (++errCount >= 20) break;
    }

    outFile << "\nError Events:\n";
    if (result.errorOffsets.empty()) {
        outFile << "No error events found.\n";
    } else {
        std::ifstream originalFile(originalFilePath);
        if (originalFile.is_open()) {
            std::string errorLine;
            for (std::streampos offset : result.errorOffsets) {
                originalFile.seekg(offset);
                std::getline(originalFile, errorLine);
                outFile << errorLine << "\n";
            }
        }
    }
    outFile.close();
    std::cout << filename << " saved.\n";
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: android_log_analyzer <log_file_path>\n";
        return 1;
    }

    try {
        std::string filePath = argv[1];

        // auto start = std::chrono::high_resolution_clock::now();

        AnalysisResult result = analyzeFileMultiThreaded(filePath);

        // auto end = std::chrono::high_resolution_clock::now();
        // std::chrono::duration<double, std::milli> diff = end - start;
        // std::cout << "Analysis Time: " << diff.count() << " ms\n";

        std::string fileName = "./out/multi_analysis_" + getCurrentTimeStr() + ".txt";
        saveResultToFile(fileName, result, filePath);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}