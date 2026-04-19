#include <array>
#include <cstdint>

#include "error.hpp"

namespace pci {
/** @brief CONFIG_ADDRESS レジスタのIOポートアドレス */
const uint16_t kConfigAddress = 0x0cf8;
/** @brief CONFIG_DATA レジスタのIOポートアドレス */
const uint16_t kConfigData = 0x0cfc;

/** @brief PCI デバイスを操作するための基礎データを格納する
 *
 * バス番号，デバイス番号，ファンクション番号はデバイスを特定するのに必須．
 * その他の情報は単に利便性のために加えてある．
 * */
struct Device {
  uint8_t bus, device, function, header_type;
};

/** @brief ScanAllBus() により発見された PCI デバイスの一覧 */
inline std::array<Device, 32> devices;

/** @brief PCI デバイスをすべて探索し devices に格納する
 *
 * バス 0 から再帰的に PCI デバイスを探索し，devices の先頭から詰めて書き込む．
 * 発見したデバイスの数を num_devices に設定する．
 */
Error ScanAllBus();
} // namespace pci
