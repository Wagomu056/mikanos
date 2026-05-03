#include <array>
#include <cstdint>

#include "error.hpp"

namespace pci {
/** @brief CONFIG_ADDRESS レジスタのIOポートアドレス */
const uint16_t kConfigAddress = 0x0cf8;
/** @brief CONFIG_DATA レジスタのIOポートアドレス */
const uint16_t kConfigData = 0x0cfc;

struct ClassCode {
  uint8_t base, sub, interface;

  bool Match(uint8_t b) { return b == base; }
  bool Match(uint8_t b, uint8_t s) { return Match(b) && s == sub; }
  bool Match(uint8_t b, uint8_t s, uint8_t i) {
    return Match(b, s) && i == interface;
  }
};

/** @brief PCI デバイスを操作するための基礎データを格納する
 *
 * バス番号，デバイス番号，ファンクション番号はデバイスを特定するのに必須．
 * その他の情報は単に利便性のために加えてある．
 * */
struct Device {
  uint8_t bus, device, function, header_type;
  ClassCode class_code;
};

uint16_t ReadVendorId(uint8_t bus, uint8_t device, uint8_t function);
inline uint16_t ReadVendorId(const Device &dev) {
  return ReadVendorId(dev.bus, dev.device, dev.function);
}

ClassCode ReadClassCode(uint8_t bus, uint8_t device, uint8_t function);

WithError<uint64_t> ReadBar(Device &device, unsigned int bar_index);

uint32_t ReadConfReg(const Device &dev, uint8_t reg_addr);
void WriteConfReg(const Device &dev, uint8_t reg_addr, uint32_t value);

struct MSICapability {
  union {
    uint32_t data;
    struct {
      uint32_t cap_id : 8;
      uint32_t next_ptr : 8;
      uint32_t msi_enable : 1;
      uint32_t multi_msg_capable : 3;
      uint32_t multi_msg_enable : 3;
      uint32_t addr_64_capable : 1;
      uint32_t per_vector_mask_capable : 1;
      uint32_t : 7;
    } __attribute__((packed)) bits;
  } __attribute__((packed)) header;

  uint32_t msg_addr;
  uint32_t msg_upper_addr;
  uint32_t msg_data;
  uint32_t mask_bits;
  uint32_t pending_bits;
} __attribute__((packed));

/** @brief PCI ケーパビリティレジスタの共通ヘッダ */
union CapabilityHeader {
  uint32_t data;
  struct {
    uint32_t cap_id : 8;
    uint32_t next_ptr : 8;
    uint32_t cap : 16;
  } __attribute__((packed)) bits;
} __attribute__((packed));

const uint8_t kCapabilityMSI = 0x05;
const uint8_t kCapabilityMSIX = 0x11;

/** @brief ScanAllBus() により発見された PCI デバイスの一覧 */
inline std::array<Device, 32> devices;

/** @brief devices の有効な要素の数 */
inline int num_device;

/** @brief PCI デバイスをすべて探索し devices に格納する
 *
 * バス 0 から再帰的に PCI デバイスを探索し，devices の先頭から詰めて書き込む．
 * 発見したデバイスの数を num_devices に設定する．
 */
Error ScanAllBus();

constexpr uint8_t CalcBarAddress(unsigned int bar_index) {
  return 0x10 + 4 * bar_index;
}

enum class MSITriggerMode { kEdge = 0, kLevel = 1 };

enum class MSIDeliveryMode {
  kFixed = 0b000,
  kLowestPriority = 0b001,
  kSMI = 0b010,
  kNMI = 0b100,
  kINIT = 0b101,
  kExtINT = 0b111,
};

Error ConfigureMSIFixedDestination(const Device &dev, uint8_t apic_id,
                                   MSITriggerMode trigger_mode,
                                   MSIDeliveryMode delivery_mode,
                                   uint8_t vector,
                                   unsigned int num_vector_exponent);

} // namespace pci
