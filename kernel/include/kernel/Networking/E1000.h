#pragma once

#include <BAN/UniqPtr.h>
#include <kernel/InterruptController.h>
#include <kernel/Networking/NetworkDriver.h>
#include <kernel/PCI.h>

#define E1000_NUM_RX_DESC 32
#define E1000_NUM_TX_DESC 8

namespace Kernel
{

	class E1000 final : public NetworkDriver, public Interruptable
	{
	public:
		static bool probe(PCI::Device&);
		static BAN::ErrorOr<BAN::UniqPtr<E1000>> create(PCI::Device&);
		~E1000();

		virtual uint8_t* get_mac_address() override { return m_mac_address; }
		virtual BAN::ErrorOr<void> send_packet(const void* data, uint16_t len) override;

		virtual bool link_up() override { return m_link_up; }
		virtual int link_speed() override;

		virtual void handle_irq() override { ASSERT_NOT_REACHED(); }

	private:
		E1000(PCI::Device& pci_device) : m_pci_device(pci_device) {}
		BAN::ErrorOr<void> initialize();

		static void interrupt_handler();

		uint32_t read32(uint16_t reg);
		void write32(uint16_t reg, uint32_t value);

		void detect_eeprom();
		uint32_t eeprom_read(uint8_t addr);
		BAN::ErrorOr<void> read_mac_address();

		void initialize_rx();
		void initialize_tx();

		void enable_link();
		BAN::ErrorOr<void> enable_interrupts();

		void handle_receive();

	private:
		PCI::Device&			m_pci_device;
		BAN::UniqPtr<PCI::BarRegion> m_bar_region;
		bool					m_has_eerprom { false };
		uint8_t					m_mac_address[6] {};
		uint16_t				m_rx_current {};
		uint16_t				m_tx_current {};
		struct e1000_rx_desc*	m_rx_descs[E1000_NUM_RX_DESC] {};
		struct e1000_tx_desc*	m_tx_descs[E1000_NUM_TX_DESC] {};
		bool					m_link_up { false };
	};

}
