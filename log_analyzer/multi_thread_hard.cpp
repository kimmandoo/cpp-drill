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
    std::string pid;
    std::string tid;
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
    size_t continuationLines = 0;
    size_t ignoredLines = 0;

    bool hasFirstRecord = false;
    LogRecord firstRecord;
    std::streampos firstRecordOffset = 0;
    bool hasLastRecord = false;
    LogRecord lastRecord;

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

    pos = line.find_first_not_of(' ', endPos);
    if (pos == std::string::npos) return false;
    endPos = line.find(' ', pos);
    if (endPos == std::string::npos) return false;
    std::string pid = line.substr(pos, endPos - pos);

    pos = line.find_first_not_of(' ', endPos);
    if (pos == std::string::npos) return false;
    endPos = line.find(' ', pos);
    if (endPos == std::string::npos) return false;
    std::string tid = line.substr(pos, endPos - pos);

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
    outRecord.pid = pid;
    outRecord.tid = tid;
    outRecord.level = convertLevel(levelChar);
    outRecord.tag = tag;
    outRecord.message = message;

    return true;
}

bool isSameHeaderContinuation(const LogRecord& previous, const LogRecord& current) {
    return previous.timestamp == current.timestamp
        && previous.pid == current.pid
        && previous.tid == current.tid
        && previous.level == current.level
        && previous.tag == current.tag;
}

void decrementCount(std::unordered_map<std::string, size_t>& counts, const std::string& key) {
    auto it = counts.find(key);
    if (it == counts.end()) return;

    if (it->second <= 1) {
        counts.erase(it);
        return;
    }

    it->second--;
}

void removeOffset(std::vector<std::streampos>& offsets, std::streampos offset) {
    auto it = std::find(offsets.begin(), offsets.end(), offset);
    if (it != offsets.end()) {
        offsets.erase(it);
    }
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

    file.seekg(startOffset);

    if (startOffset > 0) {
        std::string temp;
        std::getline(file, temp);
    }

    std::string line;
    bool hasPreviousLogHeader = false;
    LogRecord previousRecord;
    while (true) {
        std::streampos currentOffset = file.tellg();
        
        if (currentOffset == std::streampos(-1) || currentOffset >= endOffset) {
            break;
        }

        if (!std::getline(file, line)) {
            break;
        }

        result.totalLines++;
        LogRecord record;
        if(!parseLogLine(line, record)){
            if (hasPreviousLogHeader) {
                result.continuationLines++;
                continue;
            }

            result.ignoredLines++;
            continue;
        }

        if (hasPreviousLogHeader && isSameHeaderContinuation(previousRecord, record)) {
            result.continuationLines++;
            previousRecord = record;
            result.hasLastRecord = true;
            result.lastRecord = record;
            continue;
        }

        hasPreviousLogHeader = true;
        previousRecord = record;
        if (!result.hasFirstRecord) {
            result.hasFirstRecord = true;
            result.firstRecord = record;
            result.firstRecordOffset = currentOffset;
        }
        result.hasLastRecord = true;
        result.lastRecord = record;
        analyzeRecord(record, currentOffset, result);
    }

    return result;
}

AnalysisResult mergeResults(const std::vector<AnalysisResult>& results) {
    AnalysisResult global;
    bool hasPreviousChunkLastRecord = false;
    LogRecord previousChunkLastRecord;
    
    for (const auto& r : results) {
        global.totalLines += r.totalLines;
        global.parsedLines += r.parsedLines;
        global.continuationLines += r.continuationLines;
        global.ignoredLines += r.ignoredLines;

        for (const auto& [k, v] : r.levelCount) global.levelCount[k] += v;
        for (const auto& [k, v] : r.eventCount) global.eventCount[k] += v;
        for (const auto& [k, v] : r.errorEventCount) global.errorEventCount[k] += v;

        global.errorOffsets.insert(global.errorOffsets.end(), r.errorOffsets.begin(), r.errorOffsets.end());

        if (hasPreviousChunkLastRecord
            && r.hasFirstRecord
            && isSameHeaderContinuation(previousChunkLastRecord, r.firstRecord)) {
            global.parsedLines--;
            global.continuationLines++;

            decrementCount(global.levelCount, r.firstRecord.level);
            decrementCount(global.eventCount, r.firstRecord.tag);

            if (isErrorLevel(r.firstRecord.level)) {
                decrementCount(global.errorEventCount, r.firstRecord.tag);
                removeOffset(global.errorOffsets, r.firstRecordOffset);
            }
        }

        if (r.hasLastRecord) {
            hasPreviousChunkLastRecord = true;
            previousChunkLastRecord = r.lastRecord;
        }
    }

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
    outFile << "Continuation lines: " << result.continuationLines << "\n";
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
            std::streampos consumedUntil = 0;
            for (std::streampos offset : result.errorOffsets) {
                if (offset < consumedUntil) {
                    continue;
                }

                originalFile.seekg(offset);
                bool isFirstLine = true;
                LogRecord previousRecord;

                while (std::getline(originalFile, errorLine)) {
                    std::streampos nextOffset = originalFile.tellg();
                    LogRecord record;
                    bool parsed = parseLogLine(errorLine, record);

                    if (isFirstLine) {
                        if (!parsed) {
                            break;
                        }

                        previousRecord = record;
                    } else if (parsed) {
                        if (!isSameHeaderContinuation(previousRecord, record)) {
                            break;
                        }

                        previousRecord = record;
                    }

                    outFile << errorLine << "\n";
                    if (nextOffset != std::streampos(-1)) {
                        consumedUntil = nextOffset;
                    }
                    isFirstLine = false;
                }
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

        std::string fileName = "./out/multi_continuation_analysis_" + getCurrentTimeStr() + ".txt";
        saveResultToFile(fileName, result, filePath);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
