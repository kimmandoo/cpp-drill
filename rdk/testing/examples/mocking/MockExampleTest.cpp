// MockExampleTest.cpp
// Comprehensive examples of Google Mock usage in RDK testing

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <map>
#include <string>
#include <vector>
#include <memory>
#include <functional>

// ============================================================================
// Interfaces to Mock
// ============================================================================

// Network interface
class INetworkClient {
public:
    virtual ~INetworkClient() = default;
    virtual bool connect(const std::string& host, int port) = 0;
    virtual void disconnect() = 0;
    virtual bool isConnected() const = 0;
    virtual std::string sendRequest(const std::string& request) = 0;
    virtual void setTimeout(int milliseconds) = 0;
};

// Database interface
class IDatabase {
public:
    virtual ~IDatabase() = default;
    virtual bool open(const std::string& connectionString) = 0;
    virtual void close() = 0;
    virtual bool execute(const std::string& sql) = 0;
    virtual std::vector<std::map<std::string, std::string>> query(const std::string& sql) = 0;
    virtual bool beginTransaction() = 0;
    virtual bool commit() = 0;
    virtual bool rollback() = 0;
    virtual int getLastErrorCode() const = 0;
    virtual std::string getLastErrorMessage() const = 0;
};

// Logger interface
class ILogger {
public:
    enum class Level { DEBUG, INFO, WARNING, ERROR, FATAL };

    virtual ~ILogger() = default;
    virtual void log(Level level, const std::string& message) = 0;
    virtual void setLevel(Level level) = 0;
    virtual bool isEnabled(Level level) const = 0;
    virtual void flush() = 0;
};

// ============================================================================
// Mock Classes
// ============================================================================

class MockNetworkClient : public INetworkClient {
public:
    MOCK_METHOD(bool, connect, (const std::string& host, int port), (override));
    MOCK_METHOD(void, disconnect, (), (override));
    MOCK_METHOD(bool, isConnected, (), (const, override));
    MOCK_METHOD(std::string, sendRequest, (const std::string& request), (override));
    MOCK_METHOD(void, setTimeout, (int milliseconds), (override));
};

class MockDatabase : public IDatabase {
public:
    MOCK_METHOD(bool, open, (const std::string& connectionString), (override));
    MOCK_METHOD(void, close, (), (override));
    MOCK_METHOD(bool, execute, (const std::string& sql), (override));
    MOCK_METHOD(std::vector<std::map<std::string, std::string>>, query, (const std::string& sql), (override));
    MOCK_METHOD(bool, beginTransaction, (), (override));
    MOCK_METHOD(bool, commit, (), (override));
    MOCK_METHOD(bool, rollback, (), (override));
    MOCK_METHOD(int, getLastErrorCode, (), (const, override));
    MOCK_METHOD(std::string, getLastErrorMessage, (), (const, override));
};

class MockLogger : public ILogger {
public:
    MOCK_METHOD(void, log, (Level level, const std::string& message), (override));
    MOCK_METHOD(void, setLevel, (Level level), (override));
    MOCK_METHOD(bool, isEnabled, (Level level), (const, override));
    MOCK_METHOD(void, flush, (), (override));
};

// ============================================================================
// Service Under Test
// ============================================================================

class DataService {
public:
    DataService(IDatabase& db, ILogger& logger, INetworkClient& network)
        : db_(db), logger_(logger), network_(network) {}

    bool initialize() {
        logger_.log(ILogger::Level::INFO, "Initializing DataService");

        if (!db_.open("connection_string")) {
            logger_.log(ILogger::Level::ERROR, "Failed to open database");
            return false;
        }

        if (!network_.connect("api.example.com", 443)) {
            logger_.log(ILogger::Level::ERROR, "Failed to connect to network");
            db_.close();
            return false;
        }

        logger_.log(ILogger::Level::INFO, "DataService initialized successfully");
        return true;
    }

    void shutdown() {
        logger_.log(ILogger::Level::INFO, "Shutting down DataService");
        network_.disconnect();
        db_.close();
    }

    bool syncData() {
        logger_.log(ILogger::Level::INFO, "Starting data sync");

        if (!network_.isConnected()) {
            logger_.log(ILogger::Level::ERROR, "Network not connected");
            return false;
        }

        auto data = db_.query("SELECT * FROM items");
        if (data.empty()) {
            logger_.log(ILogger::Level::WARNING, "No data to sync");
            return true;
        }

        for (const auto& row : data) {
            std::string request = "UPDATE item " + row.at("id");
            std::string response = network_.sendRequest(request);
            if (response.empty()) {
                logger_.log(ILogger::Level::ERROR, "Sync failed for item " + row.at("id"));
                return false;
            }
        }

        logger_.log(ILogger::Level::INFO, "Data sync completed");
        return true;
    }

private:
    IDatabase& db_;
    ILogger& logger_;
    INetworkClient& network_;
};

// ============================================================================
// Mock Usage Examples
// ============================================================================

using ::testing::_;
using ::testing::AnyNumber;
using ::testing::AtLeast;
using ::testing::AtMost;
using ::testing::Between;
using ::testing::ByMove;
using ::testing::DoAll;
using ::testing::DoDefault;
using ::testing::ElementsAre;
using ::testing::Eq;
using ::testing::Exactly;
using ::testing::Field;
using ::testing::Ge;
using ::testing::Gt;
using ::testing::IgnoreResult;
using ::testing::InSequence;
using ::testing::Invoke;
using ::testing::InvokeWithoutArgs;
using ::testing::IsEmpty;
using ::testing::Le;
using ::testing::Lt;
using ::testing::Matcher;
using ::testing::Ne;
using ::testing::Not;
using ::testing::Pointee;
using ::testing::Property;
using ::testing::Ref;
using ::testing::Return;
using ::testing::ReturnArg;
using ::testing::ReturnNew;
using ::testing::ReturnNull;
using ::testing::ReturnPointee;
using ::testing::ReturnRef;
using ::testing::ReturnRefOfCopy;
using ::testing::SaveArg;
using ::testing::SaveArgPointee;
using ::testing::SetArgPointee;
using ::testing::SetArgReferee;
using ::testing::SetErrnoAndReturn;
using ::testing::SizeIs;
using ::testing::StartsWith;
using ::testing::StrEq;
using ::testing::StrNe;
using ::testing::Throws;
using ::testing::TypedEq;
using ::testing::WithArg;
using ::testing::WithArgs;
using ::testing::WithoutArgs;

class MockExampleTest : public ::testing::Test {
protected:
    void SetUp() override {
        mockDb_ = std::make_unique<MockDatabase>();
        mockLogger_ = std::make_unique<MockLogger>();
        mockNetwork_ = std::make_unique<MockNetworkClient>();

        service_ = std::make_unique<DataService>(
            *mockDb_, *mockLogger_, *mockNetwork_
        );
    }

    void TearDown() override {
        service_.reset();
        mockNetwork_.reset();
        mockLogger_.reset();
        mockDb_.reset();
    }

    std::unique_ptr<MockDatabase> mockDb_;
    std::unique_ptr<MockLogger> mockLogger_;
    std::unique_ptr<MockNetworkClient> mockNetwork_;
    std::unique_ptr<DataService> service_;
};

// Example 1: Basic expectations with Return
TEST_F(MockExampleTest, Initialize_WithSuccessfulDeps_ReturnsTrue) {
    // Setup expectations
    EXPECT_CALL(*mockLogger_, log(ILogger::Level::INFO, _))
        .Times(AtLeast(1));

    EXPECT_CALL(*mockDb_, open(StrEq("connection_string")))
        .WillOnce(Return(true));

    EXPECT_CALL(*mockNetwork_, connect(StrEq("api.example.com"), 443))
        .WillOnce(Return(true));

    // Execute
    bool result = service_->initialize();

    // Verify
    EXPECT_TRUE(result);
}

// Example 2: Sequential expectations with InSequence
TEST_F(MockExampleTest, Initialize_CallsMethodsInCorrectOrder) {
    {
        InSequence seq;

        EXPECT_CALL(*mockLogger_, log(ILogger::Level::INFO, StrEq("Initializing DataService")));
        EXPECT_CALL(*mockDb_, open(_));
        EXPECT_CALL(*mockNetwork_, connect(_, _));
        EXPECT_CALL(*mockLogger_, log(ILogger::Level::INFO, StrEq("DataService initialized successfully")));
    }

    EXPECT_CALL(*mockDb_, close()).Times(0);

    service_->initialize();
}

// Example 3: Multiple actions with DoAll
TEST_F(MockExampleTest, SyncData_LogsAndSendsRequests) {
    // Setup for initialization
    EXPECT_CALL(*mockLogger_, log(ILogger::Level::INFO, _)).Times(AnyNumber());
    EXPECT_CALL(*mockDb_, open(_)).WillOnce(Return(true));
    EXPECT_CALL(*mockNetwork_, connect(_, _)).WillOnce(Return(true));
    service_->initialize();

    // Setup test data
    std::vector<std::map<std::string, std::string>> mockData{
        {{"id", "1"}, {"name", "Item1"}},
        {{"id", "2"}, {"name", "Item2"}}
    };

    // Setup expectations
    EXPECT_CALL(*mockNetwork_, isConnected())
        .WillOnce(Return(true));

    EXPECT_CALL(*mockDb_, query(StrEq("SELECT * FROM items")))
        .WillOnce(Return(mockData));

    // Expect sendRequest to be called for each item
    EXPECT_CALL(*mockNetwork_, sendRequest(StartsWith("UPDATE item")))
        .Times(2)
        .WillRepeatedly(Return("OK"));

    // Execute
    bool result = service_->syncData();

    // Verify
    EXPECT_TRUE(result);
}

// Example 4: Error handling and exceptions
TEST_F(MockExampleTest, Initialize_DbOpenFails_ReturnsFalse) {
    EXPECT_CALL(*mockLogger_, log(ILogger::Level::INFO, _));
    EXPECT_CALL(*mockLogger_, log(ILogger::Level::ERROR, _));

    EXPECT_CALL(*mockDb_, open(_))
        .WillOnce(Return(false));

    EXPECT_CALL(*mockNetwork_, connect(_, _)).Times(0);

    bool result = service_->initialize();

    EXPECT_FALSE(result);
}

// Example 5: Argument capturing with SaveArg
TEST_F(MockExampleTest, Initialize_CapturesConnectionDetails) {
    std::string capturedHost;
    int capturedPort = 0;

    EXPECT_CALL(*mockLogger_, log(ILogger::Level::INFO, _)).Times(AnyNumber());
    EXPECT_CALL(*mockDb_, open(_)).WillOnce(Return(true));

    EXPECT_CALL(*mockNetwork_, connect(_, _))
        .WillOnce(DoAll(
            SaveArg<0>(&capturedHost),
            SaveArg<1>(&capturedPort),
            Return(true)
        ));

    service_->initialize();

    EXPECT_EQ(capturedHost, "api.example.com");
    EXPECT_EQ(capturedPort, 443);
}

// Example 6: Custom matcher usage
MATCHER(IsValidPort, "is a valid port number") {
    return arg > 0 && arg < 65536;
}

MATCHER_P(ContainsSubstring, substr, "contains the substring") {
    return arg.find(substr) != std::string::npos;
}

TEST_F(MockExampleTest, Initialize_UsesValidPort) {
    EXPECT_CALL(*mockLogger_, log(ILogger::Level::INFO, _)).Times(AnyNumber());
    EXPECT_CALL(*mockDb_, open(_)).WillOnce(Return(true));

    EXPECT_CALL(*mockNetwork_, connect(_, IsValidPort()))
        .WillOnce(Return(true));

    service_->initialize();
}

TEST_F(MockExampleTest, Initialize_LogsContainExpectedMessages) {
    EXPECT_CALL(*mockLogger_, log(ILogger::Level::INFO, ContainsSubstring("DataService")))
        .Times(AtLeast(1));

    EXPECT_CALL(*mockDb_, open(_)).WillOnce(Return(true));
    EXPECT_CALL(*mockNetwork_, connect(_, _)).WillOnce(Return(true));

    service_->initialize();
}

// Example 7: Delegating to real implementation
class PartialMockDatabase : public IDatabase {
public:
    MOCK_METHOD(bool, open, (const std::string& connectionString), (override));
    MOCK_METHOD(void, close, (), (override));
    MOCK_METHOD(bool, execute, (const std::string& sql), (override));
    MOCK_METHOD(std::vector<std::map<std::string, std::string>>, query, (const std::string& sql), (override));
    MOCK_METHOD(bool, beginTransaction, (), (override));
    MOCK_METHOD(bool, commit, (), (override));
    MOCK_METHOD(bool, rollback, (), (override));
    MOCK_METHOD(int, getLastErrorCode, (), (const, override));
    MOCK_METHOD(std::string, getLastErrorMessage, (), (const, override));

    // Real implementation for specific methods
    bool realOpen(const std::string& connectionString) {
        // Real logic here
        (void)connectionString;
        return true;
    }
};

TEST(PartialMockTest, CanDelegateToRealImplementation) {
    PartialMockDatabase mockDb;

    // Use ON_CALL for default behavior, EXPECT_CALL for assertions
    ON_CALL(mockDb, open(_))
        .WillByDefault(Invoke(&mockDb, &PartialMockDatabase::realOpen));

    EXPECT_CALL(mockDb, open("test_connection"));

    bool result = mockDb.open("test_connection");
    EXPECT_TRUE(result);
}

// Example 8: Mocking free functions with function objects
class FunctionMock {
public:
    MOCK_METHOD(int, calculate, (int, int));
};

// Function under test that accepts a callback
template<typename Func>
int processWithCallback(int a, int b, Func&& callback) {
    return callback(a, b);
}

TEST(FunctionMockTest, CanMockCallback) {
    FunctionMock mock;

    EXPECT_CALL(mock, calculate(5, 3))
        .WillOnce(Return(8));

    // Use lambda to bridge between mock and callback
    auto callback = [&mock](int a, int b) {
        return mock.calculate(a, b);
    };

    int result = processWithCallback(5, 3, callback);
    EXPECT_EQ(result, 8);
}
