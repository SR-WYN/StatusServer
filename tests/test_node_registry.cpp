// test_node_registry.cpp - StatusServer Token 校验与负载均衡基本场景测试
#include <gtest/gtest.h>

#include "MemoryNodeRegistryImpl.h"
#include "error_codes.h"

#include <climits>

class NodeRegistryTest : public ::testing::Test
{
protected:
    void SetUp() override { registry_ = std::make_unique<MemoryNodeRegistryImpl>(); }

    std::unique_ptr<MemoryNodeRegistryImpl> registry_;
};

TEST_F(NodeRegistryTest, SaveAndValidateUserTokenSuccess)
{
    EXPECT_TRUE(registry_->saveToken(1, "user-token-123"));
    EXPECT_EQ(registry_->validateToken(1, "user-token-123"), ErrorCodes::SUCCESS);
}

TEST_F(NodeRegistryTest, ValidateUserTokenMismatch)
{
    registry_->saveToken(1, "user-token-123");
    EXPECT_EQ(registry_->validateToken(1, "wrong-token"), ErrorCodes::TOKEN_INVALID);
}

TEST_F(NodeRegistryTest, ValidateUserTokenNotFound)
{
    EXPECT_EQ(registry_->validateToken(99, "anything"), ErrorCodes::TOKEN_INVALID);
}

TEST_F(NodeRegistryTest, ValidateFileTransferToken)
{
    registry_->saveFileToken(1, "file-token-456");
    EXPECT_EQ(registry_->validateToken(1, "file-token-456"), ErrorCodes::SUCCESS);
}

TEST_F(NodeRegistryTest, SelectLeastLoadedNode)
{
    NodeInfo a{.name = "Node-A", .client_host = "127.0.0.1", .client_port = "8001"};
    NodeInfo b{.name = "Node-B", .client_host = "127.0.0.1", .client_port = "8002"};
    NodeInfo c{.name = "Node-C", .client_host = "127.0.0.1", .client_port = "8003"};

    registry_->registerNode(a);
    registry_->registerNode(b);
    registry_->registerNode(c);

    registry_->bindUser(1, "Node-A");
    registry_->bindUser(2, "Node-B");
    registry_->bindUser(3, "Node-B");
    registry_->bindUser(4, "Node-B");

    auto selected = registry_->selectLeastLoadedNode();
    ASSERT_TRUE(selected.has_value());
    EXPECT_EQ(selected->name, "Node-C");
}

TEST_F(NodeRegistryTest, SelectLeastLoadedNodeSkipNonChatServer)
{
    NodeInfo gate{.name = "GateServer-1", .client_host = "127.0.0.1", .client_port = "8080"};
    NodeInfo chat{.name = "Node-A", .client_host = "127.0.0.1", .client_port = "8001"};

    registry_->registerNode(gate);
    registry_->registerNode(chat);

    auto selected = registry_->selectLeastLoadedNode();
    ASSERT_TRUE(selected.has_value());
    EXPECT_EQ(selected->name, "Node-A");
}

TEST_F(NodeRegistryTest, SelectLeastLoadedNodeNoNodes)
{
    auto selected = registry_->selectLeastLoadedNode();
    EXPECT_FALSE(selected.has_value());
}

TEST_F(NodeRegistryTest, BindUserIncrementsLoginCount)
{
    NodeInfo a{.name = "Node-A", .client_host = "127.0.0.1", .client_port = "8001"};
    registry_->registerNode(a);

    EXPECT_TRUE(registry_->bindUser(1, "Node-A"));

    auto selected = registry_->selectLeastLoadedNode();
    ASSERT_TRUE(selected.has_value());
    EXPECT_EQ(selected->name, "Node-A");
}

TEST_F(NodeRegistryTest, UnbindUserDecrementsLoginCount)
{
    NodeInfo a{.name = "Node-A", .client_host = "127.0.0.1", .client_port = "8001"};
    registry_->registerNode(a);

    registry_->bindUser(1, "Node-A");
    EXPECT_TRUE(registry_->unbindUser(1));

    auto selected = registry_->selectLeastLoadedNode();
    ASSERT_TRUE(selected.has_value());
    EXPECT_EQ(selected->name, "Node-A");
}
