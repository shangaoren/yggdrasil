#include <gtest/gtest.h>
#include "../../src/framework/YList.hpp"


class TestNode;

class TestListNode : public framework::YNode<TestNode>{};
class TestList : public framework::YList<TestNode>{};

class TestNode : public  TestListNode {
public:
    explicit TestNode(const uint32_t priority): priority_(priority){};
    uint32_t priority_;
};

using TestIterator = framework::YNodeIterator<TestNode>;
using ConstTestIterator = framework::YNodeConstIterator<TestNode>;



TEST(LinkedListTest, PushFront) {
    auto testList = TestList();

    auto nodeA = TestNode(1);
    auto nodeB = TestNode(2);
    auto nodeC = TestNode(3);

    testList.push_front(&nodeA);
    testList.push_front(&nodeB);
    testList.push_front(&nodeC);

    EXPECT_EQ(testList.size(), 3);
    EXPECT_EQ(testList.begin(), TestIterator(&nodeC));
    EXPECT_TRUE(TestIterator(&nodeB).next() == &nodeA);
    EXPECT_TRUE(TestIterator(&nodeB).previous() == &nodeC);
    EXPECT_TRUE(TestIterator(&nodeA).next() == nullptr);
    EXPECT_TRUE(TestIterator(&nodeA).previous() == &nodeB);
}

TEST(LinkedListTest, Contain) {
    auto testList = TestList();

    auto nodeA = TestNode(1);
    auto nodeB = TestNode(2);

    testList.push_front(&nodeA);

    EXPECT_EQ(testList.contain(&nodeA), true);
    EXPECT_EQ(testList.contain(&nodeB), false);
}


TEST(LinkedListTest, PushBack) {
    auto testList = TestList();

    auto nodeA = TestNode(1);
    auto nodeB = TestNode(2);
    auto nodeC = TestNode(3);

    testList.push_back(&nodeA);
    testList.push_back(&nodeB);
    testList.push_back(&nodeC);

    EXPECT_EQ(testList.size(), 3);
    EXPECT_EQ(testList.begin(), TestIterator(&nodeA));
    EXPECT_TRUE(TestIterator(&nodeA).next() == &nodeB);
    EXPECT_TRUE(TestIterator(&nodeB).next() == &nodeC);
    EXPECT_TRUE(TestIterator(&nodeB).previous() == &nodeA);
    EXPECT_TRUE(TestIterator(&nodeC).next() == nullptr);
    EXPECT_TRUE(TestIterator(&nodeC).previous() == &nodeB);
}

TEST(LinkedListTest, GetAndPopFront) {
    auto testList = TestList();

    auto nodeA = TestNode(1);
    auto nodeB = TestNode(2);

    testList.push_front(&nodeA);
    testList.push_front(&nodeB);

    EXPECT_EQ(testList.size(), 2);
    EXPECT_EQ(testList.begin(), TestIterator(&nodeB));
    EXPECT_TRUE(TestIterator(&nodeB).next() == &nodeA);
    EXPECT_TRUE(TestIterator(&nodeB).previous() == nullptr);
    EXPECT_TRUE(TestIterator(&nodeA).next() == nullptr);
    EXPECT_TRUE(TestIterator(&nodeA).previous() == &nodeB);


    auto result = testList.get_and_pop_front();

    EXPECT_EQ(testList.size(), 1);
    EXPECT_TRUE(result.item() == &nodeB);
    EXPECT_TRUE(result.previous() == nullptr);
    EXPECT_TRUE(result.next() == nullptr);
    EXPECT_TRUE(testList.begin() == TestIterator(&nodeA));
    EXPECT_TRUE(TestIterator(&nodeB).previous() == nullptr);
    EXPECT_TRUE(TestIterator(&nodeB).next() == nullptr);
}

TEST(LinkedListTest, InsertWhen) {
    EXPECT_TRUE(true);
}

TEST(LinkedListTest, PopFront) {
    auto testList = TestList();

    auto nodeA = TestNode(1);
    auto nodeB = TestNode(2);
    auto nodeC = TestNode(3);

    testList.push_back(&nodeA);
    testList.push_back(&nodeB);
    testList.push_back(&nodeC);

    testList.pop_front();

    EXPECT_EQ(testList.size(), 2);
    EXPECT_EQ(testList.contain(&nodeA), false);
    EXPECT_EQ(testList.contain(&nodeB), true);
    EXPECT_EQ(testList.contain(&nodeC), true);

    EXPECT_TRUE(TestIterator(&nodeA).next() == nullptr);
    EXPECT_TRUE(TestIterator(&nodeA).previous() == nullptr);
    EXPECT_TRUE(true);
}

TEST(LinkedListTest, Erase) {
    EXPECT_TRUE(true);
}

TEST(LinkedListTest, InsertAt_Middle) {
    auto testList = TestList();

    auto nodeA = TestNode(1);
    auto nodeB = TestNode(2);
    auto nodeC = TestNode(3);
    auto nodeD = TestNode(4);

    testList.push_back(&nodeA);
    testList.push_back(&nodeB);
    testList.push_back(&nodeC);

    EXPECT_EQ(testList.size(), 3);

    testList.insertAt(TestIterator(&nodeB), &nodeD);

    EXPECT_EQ(testList.size(), 4);
    EXPECT_EQ(testList.begin(), TestIterator(&nodeA));
    EXPECT_EQ(TestIterator(&nodeD).previous(), &nodeB);
    EXPECT_EQ(TestIterator(&nodeD).next(), &nodeC);
    EXPECT_EQ(TestIterator(&nodeC).previous(), &nodeD);
    EXPECT_EQ(TestIterator(&nodeB).next(), &nodeD);
}

TEST(LinkedListTest, InsertAt_End) {
    auto testList = TestList();

    auto nodeA = TestNode(1);
    auto nodeB = TestNode(2);
    auto nodeC = TestNode(3);
    auto nodeD = TestNode(4);

    testList.push_back(&nodeA);
    testList.push_back(&nodeB);
    testList.push_back(&nodeC);

    EXPECT_EQ(testList.size(), 3);

    testList.insertAt(TestIterator(&nodeC), &nodeD);

    EXPECT_EQ(testList.size(), 4);
    EXPECT_EQ(testList.begin(), TestIterator(&nodeA));
    EXPECT_EQ(TestIterator(&nodeD).previous(), &nodeC);
    EXPECT_EQ(TestIterator(&nodeD).next(), nullptr);
    EXPECT_EQ(TestIterator(&nodeC).next(), &nodeD);
}

TEST(LinkedListTest, Clear) {
    auto testList = TestList();

    auto nodeA = TestNode(1);
    auto nodeB = TestNode(2);
    auto nodeC = TestNode(3);

    testList.push_back(&nodeA);
    testList.push_back(&nodeB);
    testList.push_back(&nodeC);

    EXPECT_EQ(testList.size(), 3);

    testList.clear();

    EXPECT_EQ(testList.size(), 0);
    EXPECT_EQ(testList.begin(), TestIterator(nullptr));
}

