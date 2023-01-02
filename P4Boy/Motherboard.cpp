#include "Motherboard.h"

#include <iostream>
#include  <iomanip>

namespace P4Boy
{
	AddressRange* MakeSimpleMemoryAddressRange(Address start, Address end)
	{
		return new AddressRange(start, end, new AddressAction_SimpleMemory(end - start, start));
	}
	void Motherboard::Tick()
	{

	}

	void Motherboard::TransferOAM(Address addr, uint8_t value)
	{
		for (uint16_t i = 0; i < 0x9F; ++i)
		{
			auto byte = _mainBus.Get_8b((uint16_t(value) << 8) + i);
			_mainBus.Set_8b(0xFE00 + i, byte);
		}
	}

	void Motherboard::ConnectAddressRange(MainBus& mainBus)
	{
		mainBus.AddSingle(0xFF46, new AddressAction_SingleAction([this](Address addr, uint8_t value) { this->TransferOAM(addr, value); }));
		mainBus.AddRange(new AddressRange(0x8000, 0x9FFF, new AddressAction_SimpleMemory(0x9FFF - 0x8000, 0x8000))); // VRAM
		mainBus.AddRange(new AddressRange(0xFF80, 0xFFFE, new AddressAction_SimpleMemory(0xFFFE - 0xFF80, 0xFF80)));

		mainBus.AddRange(0xFF00, 0xFF7F, new AddressActionNoop(), 2);

		// RAM
		mainBus.AddRange(MakeSimpleMemoryAddressRange(0xC000, 0xCFFF));
		mainBus.AddRange(MakeSimpleMemoryAddressRange(0xD000, 0xDFFF));

		// OAM FE00	FE9F
		mainBus.AddRange(MakeSimpleMemoryAddressRange(0xFE00, 0xFE9F));

		// Not usable FEA0	FEFF
		mainBus.AddRange(0xFEA0, 0xFEFF, new AddressActionNoop());

		// DEBUG 
		// AUDIO $FF10	$FF26	DMG	Audio
		mainBus.AddRange(new AddressRange(0xFF10, 0xFF26, new AddressAction_SimpleMemory(0xFF26 - 0xFF10, 0xFF10)));
		// WAVE PATTERN $FF30	$FF3F	DMG	Wave pattern
		mainBus.AddRange(MakeSimpleMemoryAddressRange(0xFF30, 0xFF3F));

		// blargg's
		static uint8_t v = 0;
		mainBus.AddSingle(0xFF01, new AddressAction_DirectValue<uint8_t>(v));
		mainBus.AddSingle(0xFF02, new AddressAction_SingleAction(
			[mainBus](Address addr, uint8_t value)
			{
				if (value == 0x81) {
					std::cout << mainBus.Get_8b(0xFF01);
				}
			}
		));

	}
}