// MatcherTest.cpp
// Comprehensive examples of Google Mock matchers in RDK testing

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <limits>
#include <memory>
#include <string>
#include <vector>
#include <map>

using ::testing::_;
using ::testing::AllOf;
using ::testing::AnyOf;
using ::testing::Contains;
using ::testing::ContainsRegex;
using ::testing::DoubleEq;
using ::testing::DoubleNear;
using ::testing::Each;
using ::testing::ElementsAre;
using ::testing::EndsWith;
using ::testing::Eq;
using ::testing::Ge;
using ::testing::Gt;
using ::testing::HasSubstr;
using ::testing::IsEmpty;
using ::testing::Key;
using ::testing::Le;
using ::testing::Lt;
using ::testing::MatchesRegex;
using ::testing::NanSensitiveDoubleEq;
using ::testing::Ne;
using ::testing::Not;
using ::testing::Pair;
using ::testing::Pointwise;
using ::testing::Property;
using ::testing::Return;
using ::testing::SizeIs;
using ::testing::StartsWith;
using ::testing::StrCaseEq;
using ::testing::StrEq;
using ::testing::StrNe;
using ::testing::UnorderedElementsAre;
using ::testing::UnorderedElementsAreArray;

// ============================================================================
// Interfaces to Mock for Matcher Demonstrations
// ============================================================================

class IDataProcessor {
public:
    virtual ~IDataProcessor() = default;
    virtual bool processString(const std::string& input) = 0;
    virtual bool processInt(int value) = 0;
    virtual bool processDouble(double value) = 0;
    virtual bool processVector(const std::vector<int>& data) = 0;
    virtual bool processMap(const std::map<std::string, int>& data) = 0;
    virtual std::string getResult() const = 0;
    virtual void setConfig(const std::string& key, const std::string& value) = 0;
};

class MockDataProcessor : public IDataProcessor {
public:
    MOCK_METHOD(bool, processString, (const std::string& input), (override));
    MOCK_METHOD(bool, processInt, (int value), (override));
    MOCK_METHOD(bool, processDouble, (double value), (override));
    MOCK_METHOD(bool, processVector, (const std::vector<int>& data), (override));
    MOCK_METHOD(bool, processMap, (const std::map<std::string, int>& data), (override));
    MOCK_METHOD(std::string, getResult, (), (const, override));
    MOCK_METHOD(void, setConfig, (const std::string& key, const std::string& value), (override));
};

// ============================================================================
// Test Suite: String Matchers
// ============================================================================

class StringMatcherTest : public ::testing::Test {
protected:
    void SetUp() override {
        processor_ = std::make_unique<MockDataProcessor>();
    }

    std::unique_ptr<MockDataProcessor> processor_;
};

TEST_F(StringMatcherTest, StrEq_ExactMatch) {
    // StrEq: String equals (exact match)
    EXPECT_CALL(*processor_, processString(StrEq("hello world")))
        .WillOnce(Return(true));

    processor_->processString("hello world");
}

TEST_F(StringMatcherTest, StrNe_NotEqual) {
    // StrNe: String not equals
    EXPECT_CALL(*processor_, processString(StrNe("forbidden")))
        .WillOnce(Return(true));

    processor_->processString("allowed");
}

TEST_F(StringMatcherTest, HasSubstr_ContainsSubstring) {
    // HasSubstr: String contains substring
    EXPECT_CALL(*processor_, processString(HasSubstr("error")))
        .WillOnce(Return(true));

    processor_->processString("An error occurred");
}

TEST_F(StringMatcherTest, StartsWith_Prefix) {
    // StartsWith: String starts with prefix
    EXPECT_CALL(*processor_, processString(StartsWith("CMD:")))
        .WillOnce(Return(true));

    processor_->processString("CMD:START");
}

TEST_F(StringMatcherTest, EndsWith_Suffix) {
    // EndsWith: String ends with suffix
    EXPECT_CALL(*processor_, processString(EndsWith(".txt")))
        .WillOnce(Return(true));

    processor_->processString("document.txt");
}

TEST_F(StringMatcherTest, StrCaseEq_CaseInsensitiveEqual) {
    // StrCaseEq: Case-insensitive string equals
    EXPECT_CALL(*processor_, processString(StrCaseEq("Hello")))
        .WillOnce(Return(true));

    processor_->processString("HELLO");
}

TEST_F(StringMatcherTest, Regex_MatchesPattern) {
    // MatchesRegex: String matches regex pattern
    EXPECT_CALL(*processor_, processString(ContainsRegex("[0-9]{4}-[0-9]{2}-[0-9]{2}")))
        .WillOnce(Return(true));

    processor_->processString("Date: 2024-01-15");
}

TEST_F(StringMatcherTest, CombiningMatchers) {
    // Combine matchers using AllOf, AnyOf, Not
    EXPECT_CALL(*processor_, processString(
        AllOf(
            StartsWith("CMD:"),
            HasSubstr("EXEC"),
            EndsWith(";"),
            Not(HasSubstr("ERROR"))
        )
    )).WillOnce(Return(true));

    processor_->processString("CMD:EXECUTE task;");
}

// ============================================================================
// Test Suite: Numeric Matchers
// ============================================================================

class NumericMatcherTest : public ::testing::Test {
protected:
    void SetUp() override {
        processor_ = std::make_unique<MockDataProcessor>();
    }

    std::unique_ptr<MockDataProcessor> processor_;
};

TEST_F(NumericMatcherTest, Eq_Equal) {
    // Eq: Equal value
    EXPECT_CALL(*processor_, processInt(Eq(42)))
        .WillOnce(Return(true));

    processor_->processInt(42);
}

TEST_F(NumericMatcherTest, Ne_NotEqual) {
    // Ne: Not equal value
    EXPECT_CALL(*processor_, processInt(Ne(0)))
        .WillOnce(Return(true));

    processor_->processInt(42);
}

TEST_F(NumericMatcherTest, Gt_GreaterThan) {
    // Gt: Greater than
    EXPECT_CALL(*processor_, processInt(Gt(10)))
        .WillOnce(Return(true));

    processor_->processInt(42);
}

TEST_F(NumericMatcherTest, Ge_GreaterThanOrEqual) {
    // Ge: Greater than or equal
    EXPECT_CALL(*processor_, processInt(Ge(42)))
        .WillOnce(Return(true));

    processor_->processInt(42);
}

TEST_F(NumericMatcherTest, Lt_LessThan) {
    // Lt: Less than
    EXPECT_CALL(*processor_, processInt(Lt(100)))
        .WillOnce(Return(true));

    processor_->processInt(42);
}

TEST_F(NumericMatcherTest, Le_LessThanOrEqual) {
    // Le: Less than or equal
    EXPECT_CALL(*processor_, processInt(Le(42)))
        .WillOnce(Return(true));

    processor_->processInt(42);
}

TEST_F(NumericMatcherTest, DoubleEq_ExactMatch) {
    // DoubleEq: Exact floating point match
    EXPECT_CALL(*processor_, processDouble(DoubleEq(3.14159)))
        .WillOnce(Return(true));

    processor_->processDouble(3.14159);
}

TEST_F(NumericMatcherTest, DoubleNear_ApproximateMatch) {
    // DoubleNear: Approximate floating point match with tolerance
    EXPECT_CALL(*processor_, processDouble(DoubleNear(3.14159, 0.001)))
        .WillOnce(Return(true));

    processor_->processDouble(3.141);
}

TEST_F(NumericMatcherTest, NanValue) {
    // NanSensitiveDoubleEq: Handle NaN values
    EXPECT_CALL(*processor_, processDouble(NanSensitiveDoubleEq(
        std::numeric_limits<double>::quiet_NaN())))
        .WillOnce(Return(true));

    processor_->processDouble(std::numeric_limits<double>::quiet_NaN());
}

TEST_F(NumericMatcherTest, RangeMatchers) {
    // Check value is in a range
    EXPECT_CALL(*processor_, processInt(AllOf(Ge(1), Le(100))))
        .WillOnce(Return(true));

    processor_->processInt(50);
}

// ============================================================================
// Test Suite: Container Matchers
// ============================================================================

class ContainerMatcherTest : public ::testing::Test {
protected:
    void SetUp() override {
        processor_ = std::make_unique<MockDataProcessor>();
    }

    std::unique_ptr<MockDataProcessor> processor_;
};

TEST_F(ContainerMatcherTest, IsEmpty) {
    // IsEmpty: Container is empty
    EXPECT_CALL(*processor_, processVector(IsEmpty()))
        .WillOnce(Return(true));

    processor_->processVector({});
}

TEST_F(ContainerMatcherTest, SizeIs_ExactSize) {
    // SizeIs: Container has exact size
    EXPECT_CALL(*processor_, processVector(SizeIs(3)))
        .WillOnce(Return(true));

    processor_->processVector({1, 2, 3});
}

TEST_F(ContainerMatcherTest, ElementsAre_ExactElements) {
    // ElementsAre: Container has exact elements in order
    EXPECT_CALL(*processor_, processVector(ElementsAre(1, 2, 3)))
        .WillOnce(Return(true));

    processor_->processVector({1, 2, 3});
}

TEST_F(ContainerMatcherTest, UnorderedElementsAre_IgnoresOrder) {
    // UnorderedElementsAre: Container has elements (order not important)
    EXPECT_CALL(*processor_, processVector(UnorderedElementsAre(1, 2, 3)))
        .WillOnce(Return(true));

    processor_->processVector({3, 1, 2});
}

TEST_F(ContainerMatcherTest, Contains_SingleElement) {
    // Contains: Container contains element
    EXPECT_CALL(*processor_, processVector(Contains(5)))
        .WillOnce(Return(true));

    processor_->processVector({1, 5, 10});
}

TEST_F(ContainerMatcherTest, Each_AllElementsMatch) {
    // Each: All elements match the inner matcher
    EXPECT_CALL(*processor_, processVector(Each(Gt(0))))
        .WillOnce(Return(true));

    processor_->processVector({1, 5, 100});
}

TEST_F(ContainerMatcherTest, Pointwise_ElementWiseComparison) {
    // Pointwise: Element-wise comparison using matchers
    EXPECT_CALL(*processor_, processVector(Pointwise(Gt(), {0, 1, 2})))
        .WillOnce(Return(true));

    processor_->processVector({1, 5, 10});  // 1>0, 5>1, 10>2
}

// ============================================================================
// Test Suite: Map Matchers
// ============================================================================

TEST_F(ContainerMatcherTest, Map_ContainsKey) {
    // Key: Map contains key
    EXPECT_CALL(*processor_, processMap(Contains(Key("name"))))
        .WillOnce(Return(true));

    processor_->processMap({{"name", 1}, {"age", 25}});
}

TEST_F(ContainerMatcherTest, Map_ValueMatches) {
    // Pair: Key-value pair matches
    EXPECT_CALL(*processor_, processMap(Contains(Pair("age", Gt(18)))))
        .WillOnce(Return(true));

    processor_->processMap({{"name", 1}, {"age", 25}});
}

TEST_F(ContainerMatcherTest, Map_ElementsArePairs) {
    // UnorderedElementsAreArray: Map elements match
    using PairType = std::pair<const std::string, int>;
    EXPECT_CALL(*processor_, processMap(UnorderedElementsAreArray({
        PairType{"a", 1},
        PairType{"b", 2}
    })))
        .WillOnce(Return(true));

    processor_->processMap({{"a", 1}, {"b", 2}});
}
