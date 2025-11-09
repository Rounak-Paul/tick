#include "../src/core/dynamic_array.h"
#include "../src/core/string.h"
#include "../src/core/hash_map.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/time.h>

using namespace Tick;

int tests_passed = 0;
int tests_failed = 0;

void assert_true(bool condition, const char* test_name) {
    if (condition) {
        tests_passed++;
        printf("✓ %s\n", test_name);
    } else {
        tests_failed++;
        printf("✗ %s\n", test_name);
    }
}

void test_dynamic_array() {
    printf("\n=== DynamicArray Tests ===\n");
    
    DynamicArray<int> arr;
    assert_true(arr.size() == 0, "Empty array has size 0");
    
    arr.push(10);
    arr.push(20);
    arr.push(30);
    assert_true(arr.size() == 3, "Array size after 3 pushes");
    assert_true(arr[0] == 10, "First element");
    assert_true(arr[1] == 20, "Second element");
    assert_true(arr[2] == 30, "Third element");
    
    arr.reserve(100);
    assert_true(arr.capacity() >= 100, "Reserve capacity");
    assert_true(arr.size() == 3, "Size unchanged after reserve");
    
    arr.clear();
    assert_true(arr.size() == 0, "Clear empties array");
    
    for (int i = 0; i < 1000; i++) {
        arr.push(i);
    }
    assert_true(arr.size() == 1000, "Large array push");
    
    int sum = 0;
    for (size_t i = 0; i < arr.size(); i++) {
        sum += arr[i];
    }
    assert_true(sum == 499500, "Array iteration sum");
}

void test_string() {
    printf("\n=== String Tests ===\n");
    
    String s1("hello");
    assert_true(s1.length() == 5, "String length");
    assert_true(strcmp(s1.c_str(), "hello") == 0, "String content");
    
    String s2("world");
    assert_true(s1 == "hello", "String equality with const char*");
    assert_true(!(s1 == s2), "String inequality");
    
    String s3(s1);
    assert_true(s3 == s1, "String copy constructor");
    
    String s4("test");
    s4 = s2;
    assert_true(s4 == s2, "String assignment operator");
    
    String empty;
    assert_true(empty.length() == 0, "Empty string length");
    assert_true(strcmp(empty.c_str(), "") == 0, "Empty string content");
}

void test_hash_map() {
    printf("\n=== HashMap Tests ===\n");
    
    HashMap<int, int> map;
    
    map.insert(1, 100);
    map.insert(2, 200);
    map.insert(3, 300);
    
    assert_true(map.size() == 3, "HashMap size");
    assert_true(map.contains(1), "Contains key 1");
    assert_true(map.contains(2), "Contains key 2");
    assert_true(map.contains(3), "Contains key 3");
    assert_true(!map.contains(4), "Does not contain key 4");
    
    int* val1 = map.find(1);
    assert_true(val1 != nullptr && *val1 == 100, "Find key 1");
    
    int* val2 = map.find(2);
    assert_true(val2 != nullptr && *val2 == 200, "Find key 2");
    
    int* val_missing = map.find(99);
    assert_true(val_missing == nullptr, "Find missing key");
    
    for (int i = 0; i < 100; i++) {
        map.insert(i + 10, i * 10);
    }
    assert_true(map.size() == 103, "HashMap after many inserts");
    
    int* val50 = map.find(50);
    assert_true(val50 != nullptr && *val50 == 400, "Find after rehash");
}

void test_string_operations() {
    printf("\n=== String Operations Tests ===\n");
    
    String s1("abc");
    String s2("abc");
    String s3("def");
    
    assert_true(s1 == s2, "String equality");
    assert_true(!(s1 == s3), "String inequality");
    
    assert_true(s1[0] == 'a', "String indexing [0]");
    assert_true(s1[1] == 'b', "String indexing [1]");
    assert_true(s1[2] == 'c', "String indexing [2]");
    
    String long_str("This is a longer string for testing purposes");
    assert_true(long_str.length() == 44, "Long string length");
}

void benchmark_dynamic_array() {
    printf("\n=== DynamicArray Performance ===\n");
    
    struct timeval start, end;
    
    gettimeofday(&start, nullptr);
    DynamicArray<int> arr;
    for (int i = 0; i < 1000000; i++) {
        arr.push(i);
    }
    gettimeofday(&end, nullptr);
    
    long seconds = end.tv_sec - start.tv_sec;
    long micros = end.tv_usec - start.tv_usec;
    double elapsed = seconds + micros / 1000000.0;
    
    printf("  1M pushes: %.3f ms (%.0f ops/sec)\n", 
           elapsed * 1000, 1000000.0 / elapsed);
    
    gettimeofday(&start, nullptr);
    long long sum = 0;
    for (size_t i = 0; i < arr.size(); i++) {
        sum += arr[i];
    }
    gettimeofday(&end, nullptr);
    
    seconds = end.tv_sec - start.tv_sec;
    micros = end.tv_usec - start.tv_usec;
    elapsed = seconds + micros / 1000000.0;
    
    printf("  1M reads: %.3f ms (%.0f ops/sec)\n", 
           elapsed * 1000, 1000000.0 / elapsed);
    printf("  Sum verification: %lld\n", sum);
}

void benchmark_hash_map() {
    printf("\n=== HashMap Performance ===\n");
    
    struct timeval start, end;
    HashMap<int, int> map;
    
    gettimeofday(&start, nullptr);
    for (int i = 0; i < 100000; i++) {
        map.insert(i, i * 2);
    }
    gettimeofday(&end, nullptr);
    
    long seconds = end.tv_sec - start.tv_sec;
    long micros = end.tv_usec - start.tv_usec;
    double elapsed = seconds + micros / 1000000.0;
    
    printf("  100K inserts: %.3f ms (%.0f ops/sec)\n", 
           elapsed * 1000, 100000.0 / elapsed);
    
    gettimeofday(&start, nullptr);
    int found_count = 0;
    for (int i = 0; i < 100000; i++) {
        if (map.contains(i)) {
            found_count++;
        }
    }
    gettimeofday(&end, nullptr);
    
    seconds = end.tv_sec - start.tv_sec;
    micros = end.tv_usec - start.tv_usec;
    elapsed = seconds + micros / 1000000.0;
    
    printf("  100K lookups: %.3f ms (%.0f ops/sec)\n", 
           elapsed * 1000, 100000.0 / elapsed);
    printf("  Found: %d/100000\n", found_count);
}

int main() {
    printf("╔═══════════════════════════════════════════════╗\n");
    printf("║   Tick Core Data Structures Test Suite       ║\n");
    printf("╚═══════════════════════════════════════════════╝\n");
    
    test_dynamic_array();
    test_string();
    test_hash_map();
    test_string_operations();
    
    benchmark_dynamic_array();
    benchmark_hash_map();
    
    printf("\n╔═══════════════════════════════════════════════╗\n");
    printf("║   Test Results                                ║\n");
    printf("╠═══════════════════════════════════════════════╣\n");
    printf("║   Passed: %-3d                                 ║\n", tests_passed);
    printf("║   Failed: %-3d                                 ║\n", tests_failed);
    printf("║   Total:  %-3d                                 ║\n", tests_passed + tests_failed);
    printf("╚═══════════════════════════════════════════════╝\n");
    
    return tests_failed > 0 ? 1 : 0;
}
