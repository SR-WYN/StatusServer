// test_utils.cpp - StatusServer 纯函数工具类单元测试
#include <gtest/gtest.h>
#include "utils.h"
#include "defer.h"
#include <spdlog/common.h>

// ==================== utils::url ====================

class UrlEncodeTest : public ::testing::Test
{
};

TEST_F(UrlEncodeTest, PlainAscii)
{
    EXPECT_EQ(utils::url::encode("hello"), "hello");
}

TEST_F(UrlEncodeTest, Space)
{
    EXPECT_EQ(utils::url::encode("hello world"), "hello%20world");
}

TEST_F(UrlEncodeTest, SpecialChars)
{
    EXPECT_EQ(utils::url::encode("a+b=c&d"), "a%2Bb%3Dc%26d");
}

class UrlDecodeTest : public ::testing::Test
{
};

TEST_F(UrlDecodeTest, RoundTrip)
{
    std::string original = "hello world! 你好";
    EXPECT_EQ(utils::url::decode(utils::url::encode(original)), original);
}

// ==================== utils::log::parseLevel ====================

class ParseLevelTest : public ::testing::Test
{
};

TEST_F(ParseLevelTest, Info)
{
    EXPECT_EQ(utils::log::parseLevel("info"), spdlog::level::info);
}

TEST_F(ParseLevelTest, UnknownDefaultsToInfo)
{
    EXPECT_EQ(utils::log::parseLevel("verbose"), spdlog::level::info);
}

// ==================== utils::Defer ====================

class DeferTest : public ::testing::Test
{
};

TEST_F(DeferTest, ExecutesOnScopeExit)
{
    bool called = false;
    {
        utils::Defer d([&called]() { called = true; });
        EXPECT_FALSE(called);
    }
    EXPECT_TRUE(called);
}
