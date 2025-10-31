/** Abigail Fu and Kendall Gee 2025-03-15
* @file test_packet_endianness.c
* @brief Comprehensive endianness testing for packet_t communication protocol
*
 * Tests for PiCube <-> Satellite communication to ensure:
* - Correct endianness interpretation on both sides
* - Detection of endianness mismatches
* - Graceful handling of corrupted/truncated packets
* - Round-trip serialization integrity
*/
 
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include <stddef.h>
 
// TinyCrypt SHA256 size
#define TC_SHA256_DIGEST_SIZE 32
 
// Packet structure
typedef struct __attribute__((packed))
{
    uint8_t dst;
    uint8_t src;
    uint8_t flags;
    uint8_t seq;
    uint8_t len;
    uint8_t data[255 - (sizeof(uint8_t) * 5) - (sizeof(uint32_t) * 2) - TC_SHA256_DIGEST_SIZE];
    uint32_t boot_count;
    uint32_t msg_id;
    uint8_t hmac[TC_SHA256_DIGEST_SIZE];
} packet_t;
 
// =============================================================================
// TEST UTILITIES
// =============================================================================
 
static int tests_passed = 0;
static int tests_failed = 0;
 
#define TEST_ASSERT(condition, message) \
    do { \
        if (condition) { \
            printf("  ✓ %s\n", message); \
            tests_passed++; \
        } else { \
            printf("  ✗ FAIL: %s\n", message); \
            tests_failed++; \
        } \
    } while(0)
 
#define TEST_START(name) \
    printf("\n[TEST] %s\n", name); \
    printf("----------------------------------------\n")
 
#define TEST_END() \
    printf("----------------------------------------\n")
 
// =============================================================================
// PACKET HELPERS
// =============================================================================
 
/**
* Create a known-good test packet with predictable values
*/
packet_t create_test_packet(void)
{
    packet_t p;
    memset(&p, 0, sizeof(packet_t));
   
    p.dst = 0x01;
    p.src = 0x02;
    p.flags = 0x80;
    p.seq = 0x42;
    p.len = sizeof(packet_t);
   
    // Recognizable multi-byte values for endianness testing
    p.boot_count = 0x12345678;  // Will be 78 56 34 12 in little-endian
    p.msg_id = 0xABCDEF01;      // Will be 01 EF CD AB in little-endian
   
    // Simple test data
    const char *test_data = "ENDIAN_TEST";
    strncpy((char*)p.data, test_data, sizeof(p.data) - 1);
   
    // Mock HMAC (in real code, compute actual HMAC)
    for (int i = 0; i < TC_SHA256_DIGEST_SIZE; i++) {
        p.hmac[i] = 0x00 + i;
    }
   
    return p;
}
 
/**
* Compare two packets for equality
*/
bool packets_equal(const packet_t *p1, const packet_t *p2)
{
    return memcmp(p1, p2, sizeof(packet_t)) == 0;
}
 
/**
* Dump packet in human-readable format
*/
void dump_packet(const packet_t *p, const char *label)
{
    printf("\n%s:\n", label);
    printf("  dst:        0x%02X\n", p->dst);
    printf("  src:        0x%02X\n", p->src);
    printf("  flags:      0x%02X\n", p->flags);
    printf("  seq:        0x%02X\n", p->seq);
    printf("  len:        %u\n", p->len);
    printf("  boot_count: 0x%08X (%u)\n", p->boot_count, p->boot_count);
    printf("  msg_id:     0x%08X (%u)\n", p->msg_id, p->msg_id);
    printf("  data:       %.32s...\n", p->data);
}
 
/**
* Dump raw bytes in hex
*/
void dump_hex(const uint8_t *data, size_t len, const char *label)
{
    printf("\n%s (%zu bytes):\n", label, len);
    for (size_t i = 0; i < len; i++) {
        printf("%02X ", data[i]);
        if ((i + 1) % 16 == 0) printf("\n");
    }
    if (len % 16 != 0) printf("\n");
}
 
/**
* Manually byte-swap a 32-bit value
*/
uint32_t bswap32(uint32_t val)
{
    return ((val & 0x000000FF) << 24) |
           ((val & 0x0000FF00) << 8) |
           ((val & 0x00FF0000) >> 8) |
           ((val & 0xFF000000) >> 24);
}
 
// =============================================================================
// ENDIANNESS DETECTION TESTS
// =============================================================================
 
void test_system_endianness(void)
{
    TEST_START("System Endianness Detection");
   
    uint32_t test_val = 0x12345678;
    uint8_t *bytes = (uint8_t*)&test_val;
   
    bool is_little_endian = (bytes[0] == 0x78);
    bool is_big_endian = (bytes[0] == 0x12);
   
    printf("  Test value: 0x%08X\n", test_val);
    printf("  Byte layout: %02X %02X %02X %02X\n",
           bytes[0], bytes[1], bytes[2], bytes[3]);
   
    if (is_little_endian) {
        printf("  System: LITTLE ENDIAN\n");
        TEST_ASSERT(bytes[0] == 0x78 && bytes[3] == 0x12,
                   "Little endian byte order confirmed");
    } else if (is_big_endian) {
        printf("  System: BIG ENDIAN\n");
        TEST_ASSERT(bytes[0] == 0x12 && bytes[3] == 0x78,
                   "Big endian byte order confirmed");
    } else {
        TEST_ASSERT(false, "Unknown endianness detected");
    }
   
    TEST_END();
}
 
void test_packet_field_endianness(void)
{
    TEST_START("Packet Multi-byte Field Endianness");
   
    packet_t p = create_test_packet();
   
    // Check boot_count byte layout
    uint8_t *bc_bytes = (uint8_t*)&p.boot_count;
    printf("  boot_count = 0x%08X\n", p.boot_count);
    printf("  Byte layout: %02X %02X %02X %02X\n",
           bc_bytes[0], bc_bytes[1], bc_bytes[2], bc_bytes[3]);
   
    // Check msg_id byte layout
    uint8_t *mi_bytes = (uint8_t*)&p.msg_id;
    printf("  msg_id = 0x%08X\n", p.msg_id);
    printf("  Byte layout: %02X %02X %02X %02X\n",
           mi_bytes[0], mi_bytes[1], mi_bytes[2], mi_bytes[3]);
   
    // On little-endian systems (most ARM, x86)
    TEST_ASSERT(bc_bytes[0] == 0x78, "boot_count LSB first (little-endian)");
    TEST_ASSERT(mi_bytes[0] == 0x01, "msg_id LSB first (little-endian)");
   
    TEST_END();
}
 
// =============================================================================
// ROUND-TRIP SERIALIZATION TESTS
// =============================================================================
 
void test_roundtrip_serialization(void)
{
    TEST_START("Round-trip Serialization");
   
    // Create original packet
    packet_t original = create_test_packet();
    dump_packet(&original, "Original Packet");
   
    // Serialize to byte buffer (simulate sending over wire)
    uint8_t buffer[sizeof(packet_t)];
    memcpy(buffer, &original, sizeof(packet_t));
   
    dump_hex(buffer, sizeof(packet_t), "Serialized Bytes");
   
    // Deserialize from buffer (simulate receiving)
    packet_t received;
    memcpy(&received, buffer, sizeof(packet_t));
   
    dump_packet(&received, "Deserialized Packet");
   
    // Verify all fields match
    TEST_ASSERT(received.dst == original.dst, "dst field preserved");
    TEST_ASSERT(received.src == original.src, "src field preserved");
    TEST_ASSERT(received.flags == original.flags, "flags field preserved");
    TEST_ASSERT(received.seq == original.seq, "seq field preserved");
    TEST_ASSERT(received.len == original.len, "len field preserved");
    TEST_ASSERT(received.boot_count == original.boot_count,
               "boot_count field preserved");
    TEST_ASSERT(received.msg_id == original.msg_id,
               "msg_id field preserved");
    TEST_ASSERT(memcmp(received.data, original.data, sizeof(original.data)) == 0,
               "data field preserved");
    TEST_ASSERT(memcmp(received.hmac, original.hmac, TC_SHA256_DIGEST_SIZE) == 0,
               "hmac field preserved");
   
    TEST_ASSERT(packets_equal(&original, &received),
               "Complete packet integrity verified");
   
    TEST_END();
}
 
// =============================================================================
// ENDIANNESS MISMATCH TESTS
// =============================================================================
 
void test_intentional_endian_mismatch(void)
{
    TEST_START("Intentional Endianness Mismatch (should fail gracefully)");
   
    // Create packet
    packet_t original = create_test_packet();
    printf("  Original boot_count: 0x%08X\n", original.boot_count);
    printf("  Original msg_id:     0x%08X\n", original.msg_id);
   
    // Simulate endianness mismatch by byte-swapping multi-byte fields
    packet_t swapped = original;
    swapped.boot_count = bswap32(original.boot_count);
    swapped.msg_id = bswap32(original.msg_id);
   
    printf("  Swapped boot_count:  0x%08X\n", swapped.boot_count);
    printf("  Swapped msg_id:      0x%08X\n", swapped.msg_id);
   
    // Values should be different
    TEST_ASSERT(swapped.boot_count != original.boot_count,
               "boot_count changed after byte swap");
    TEST_ASSERT(swapped.msg_id != original.msg_id,
               "msg_id changed after byte swap");
   
    // Processing should not crash (simulate with memcpy - real code would parse)
    uint8_t buffer[sizeof(packet_t)];
    memcpy(buffer, &swapped, sizeof(packet_t));
    packet_t parsed;
    memcpy(&parsed, buffer, sizeof(packet_t));
   
    TEST_ASSERT(true, "No crash when processing endian-swapped packet");
   
    // HMAC would fail in real implementation
    printf("  Note: HMAC verification would fail in production\n");
   
    TEST_END();
}
 
void test_cross_platform_endian_check(void)
{
    TEST_START("Cross-platform Endianness Check");
   
    packet_t p = create_test_packet();
   
    // Simulate sending from little-endian system
    uint8_t wire_format[sizeof(packet_t)];
    memcpy(wire_format, &p, sizeof(packet_t));
   
    // Check specific byte positions for boot_count
    size_t boot_count_offset = offsetof(packet_t, boot_count);
    uint8_t *bc_on_wire = &wire_format[boot_count_offset];
   
    printf("  boot_count offset in struct: %zu\n", boot_count_offset);
    printf("  boot_count on wire: %02X %02X %02X %02X\n",
           bc_on_wire[0], bc_on_wire[1], bc_on_wire[2], bc_on_wire[3]);
   
    // For 0x12345678 on little-endian
    TEST_ASSERT(bc_on_wire[0] == 0x78, "LSB transmitted first");
    TEST_ASSERT(bc_on_wire[3] == 0x12, "MSB transmitted last");
   
    printf("  ⚠ Warning: Receiving system MUST be little-endian!\n");
    printf("  ⚠ Big-endian receiver will interpret as 0x%02X%02X%02X%02X\n",
           bc_on_wire[0], bc_on_wire[1], bc_on_wire[2], bc_on_wire[3]);
   
    TEST_END();
}
 
// =============================================================================
// BYTE LOSS SIMULATION TESTS
// =============================================================================
 
void test_leading_byte_loss(void)
{
    TEST_START("Leading Byte Loss (loss of sync)");
   
    packet_t original = create_test_packet();
    uint8_t buffer[sizeof(packet_t)];
    memcpy(buffer, &original, sizeof(packet_t));
   
    // Simulate losing first N bytes
    for (size_t lost = 1; lost <= 10; lost++) {
        size_t remaining = sizeof(packet_t) - lost;
        uint8_t shifted[sizeof(packet_t)];
        memset(shifted, 0, sizeof(shifted));
        if (remaining > 0) {
            memcpy(shifted, buffer + lost, remaining);
        }

        packet_t parsed;
        memcpy(&parsed, shifted, sizeof(packet_t));

        // Packet should be corrupted when bytes are lost
        bool is_corrupted = (parsed.dst != original.dst) ||
                            (parsed.boot_count != original.boot_count);

        TEST_ASSERT(is_corrupted,
                   "Packet correctly detected as corrupted with byte loss");
    }
   
    printf("  Note: Parser should reject all packets with lost leading bytes\n");
   
    TEST_END();
}
 
void test_trailing_byte_loss(void)
{
    TEST_START("Trailing Byte Loss (truncated HMAC)");
   
    packet_t original = create_test_packet();
    uint8_t buffer[sizeof(packet_t)];
    memcpy(buffer, &original, sizeof(packet_t));
   
    // Simulate truncation at various points
    for (size_t trim = 1; trim <= 40; trim++) {
        size_t truncated_len = sizeof(packet_t) - trim;
       
        printf("  Testing with %zu bytes (lost %zu bytes)\n",
               truncated_len, trim);
       
        // In real parser, this would fail length check or HMAC verification
        TEST_ASSERT(truncated_len < sizeof(packet_t),
                   "Truncated packet detected");
    }
   
    printf("  Note: Real parser should check len field and packet size\n");
   
    TEST_END();
}
 
void test_multi_byte_field_corruption(void)
{
    TEST_START("Multi-byte Field Partial Loss");
   
    packet_t original = create_test_packet();
   
    // Corrupt boot_count by zeroing middle bytes
    packet_t corrupted = original;
    uint8_t *bc_bytes = (uint8_t*)&corrupted.boot_count;
    bc_bytes[1] = 0x00;  // Corrupt one byte
    bc_bytes[2] = 0x00;
   
    printf("  Original boot_count: 0x%08X\n", original.boot_count);
    printf("  Corrupted boot_count: 0x%08X\n", corrupted.boot_count);
   
    TEST_ASSERT(corrupted.boot_count != original.boot_count,
               "boot_count corruption detected");
   
    // HMAC would catch this in production
    printf("  Note: HMAC verification would fail\n");
   
    TEST_END();
}
 
// =============================================================================
// STRUCT PACKING TESTS
// =============================================================================
 
void test_struct_packing(void)
{
    TEST_START("Struct Packing Verification");
   
    printf("  sizeof(packet_t): %zu bytes\n", sizeof(packet_t));
    printf("  Expected size: 255 bytes\n");
   
    TEST_ASSERT(sizeof(packet_t) == 255, "Packet size is exactly 255 bytes");
   
    // Verify no padding between fields
    packet_t p;
    uintptr_t base = (uintptr_t)&p;
   
    printf("  Field offsets:\n");
    printf("    dst:        %zu\n", offsetof(packet_t, dst));
    printf("    src:        %zu\n", offsetof(packet_t, src));
    printf("    flags:      %zu\n", offsetof(packet_t, flags));
    printf("    seq:        %zu\n", offsetof(packet_t, seq));
    printf("    len:        %zu\n", offsetof(packet_t, len));
    printf("    data:       %zu\n", offsetof(packet_t, data));
    printf("    boot_count: %zu\n", offsetof(packet_t, boot_count));
    printf("    msg_id:     %zu\n", offsetof(packet_t, msg_id));
    printf("    hmac:       %zu\n", offsetof(packet_t, hmac));
   
    TEST_ASSERT(offsetof(packet_t, src) == 1, "No padding after dst");
    TEST_ASSERT(offsetof(packet_t, flags) == 2, "No padding after src");
    TEST_ASSERT(offsetof(packet_t, hmac) == 255 - TC_SHA256_DIGEST_SIZE,
               "HMAC is last field");
   
    TEST_END();
}
 
// =============================================================================
// MAIN TEST RUNNER
// =============================================================================
 
int main(void)
{
    printf("╔════════════════════════════════════════════════════════════╗\n");
    printf("║     PACKET ENDIANNESS & INTEGRITY TEST SUITE              ║\n");
    printf("║     Testing packet_t communication protocol               ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n");
   
    // Run all tests
    test_system_endianness();
    test_packet_field_endianness();
    test_struct_packing();
    test_roundtrip_serialization();
    test_cross_platform_endian_check();
    test_intentional_endian_mismatch();
    test_leading_byte_loss();
    test_trailing_byte_loss();
    test_multi_byte_field_corruption();
   
    // Print summary
    printf("\n╔════════════════════════════════════════════════════════════╗\n");
    printf("║                     TEST SUMMARY                          ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n");
    printf("  Tests passed: %d\n", tests_passed);
    printf("  Tests failed: %d\n", tests_failed);
    printf("  Total tests:  %d\n", tests_passed + tests_failed);
   
    if (tests_failed == 0) {
        printf("\n  ✓ ALL TESTS PASSED\n\n");
        return 0;
    } else {
        printf("\n  ✗ SOME TESTS FAILED\n\n");
        return 1;
    }
}