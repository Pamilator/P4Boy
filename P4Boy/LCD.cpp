#include "LCD.h"

#include "MainBus.h"
#include "AddressAction.h"
#include "CPU.h"


namespace P4Boy
{
	LCD::LCD() : _window(sf::VideoMode(160, 144), "SFML works!"), _mainBus(nullptr)
	{
	}
	
	void LCD::ConnectAddressRange(MainBus& mainBus)
	{
		mainBus.AddSingle(0xFF40, new AddressAction_DirectValue<LCDC>(this->_LCDC), "LCDC");
		mainBus.AddSingle(0xFF41, new AddressAction_DirectValue<LCDS>(this->_LCDS), "LCDS");

		mainBus.AddSingle(0xFF44, new AddressAction_DirectValue<uint8_t>(this->_LY), "LY");
		mainBus.AddSingle(0xFF45, new AddressAction_DirectValue<uint8_t>(this->_LYC), "LYC");

		mainBus.AddSingle(0xFF42, new AddressAction_DirectValue<uint8_t>(this->_SCY), "SCY");
		mainBus.AddSingle(0xFF43, new AddressAction_DirectValue<uint8_t>(this->_SCX), "SCX");

		mainBus.AddSingle(0xFF4A, new AddressAction_DirectValue<uint8_t>(this->_WY), "WY");
		mainBus.AddSingle(0xFF4B, new AddressAction_DirectValue<uint8_t>(this->_WX), "WX");

		mainBus.AddSingle(0xFF47, new AddressAction_DirectValue<BGP>(this->_BGP), "BGP");
		mainBus.AddSingle(0xFF48, new AddressAction_DirectValue<OBP>(this->_OBP0), "OBP0");
		mainBus.AddSingle(0xFF49, new AddressAction_DirectValue<OBP>(this->_OBP1), "OBP1");
		_mainBus = &mainBus;
	}

	void LCD::DrawBackground()
	{
		uint16_t tile_data_addr = 0x8000;
		uint16_t time_map_addr = 0x9800;

		sf::Color colors[] = {{0x08, 0x18, 0x28}, {0x34, 0x68, 0x56}, {0x88, 0xC0, 0x70}, {0xE0, 0xF8, 0xD0}};
		sf::VertexArray vertex;
		if (_LCDC.BGWEnable_Priority)
		{
			for (uint8_t y = 0; y < 32; ++y)
			{
				for (uint8_t x = 0; x < 32; ++x)
				{
					uint8_t tileIndex = _mainBus->Get_8b(time_map_addr + x + y * 32);
					if (!tileIndex)
						continue;
					uint16_t tileAddr = tile_data_addr + tileIndex * 16;
					for (uint8_t y_tile = 0; y_tile < 8; ++y_tile)
					{
						uint8_t pixels1 = _mainBus->Get_8b(tileAddr + (y_tile * 2));
						uint8_t pixels2 = _mainBus->Get_8b(tileAddr + (y_tile * 2 + 1));
						for (uint8_t x_tile = 0; x_tile <= 8; ++x_tile)
						{
							uint8_t color = ((pixels1 >> (7 - x_tile)) & 1) | (((pixels2 >> (7 - x_tile)) & 1) << 1);
							int coordX = x * 8 + x_tile - _SCX;
							int coordY = y * 8 + y_tile - _SCY;
							if (!(coordX >= 0 && coordX <= 160 && coordY >= 0 && coordY <= 144))
								continue;
							vertex.append(sf::Vertex(sf::Vector2f(coordX, coordY), colors[color]));
						}
					}
				}
			}
		}
		

		if (_LCDC.WindowEnable)
		{
			_LCDC.WindowEnable = _LCDC.WindowEnable.Get();
		}
		_window.draw(vertex);
	}

	void LCD::DrawSprites()
	{
		if (!_LCDC.OBJEnable)
			return;
		uint16_t tile_data_addr = 0x8000;
		uint16_t oam_addr = 0xFE00;

		sf::Color colors[] = { {0x08, 0x18, 0x28}, {0x34, 0x68, 0x56}, {0x88, 0xC0, 0x70}, {0xE0, 0xF8, 0xD0} };
		sf::VertexArray vertex;
		for (uint8_t i = 0; i < 40; ++i)
		{
			uint16_t spriteAddr = oam_addr + i * 4;

			uint8_t yPos = _mainBus->Get_8b(spriteAddr + 0) - 16;
			uint8_t xPos = _mainBus->Get_8b(spriteAddr + 1) - 8;
			uint8_t tileIndex = _mainBus->Get_8b(spriteAddr + 2);
			uint8_t attributes = _mainBus->Get_8b(spriteAddr + 3);

			if (tileIndex)
			{
				uint16_t tileAddr = tile_data_addr + tileIndex * 16;
				for (uint8_t y_tile = 0; y_tile < 8; ++y_tile)
				{
					uint8_t pixels1 = _mainBus->Get_8b(tileAddr + (y_tile * 2));
					uint8_t pixels2 = _mainBus->Get_8b(tileAddr + (y_tile * 2 + 1));
					for (uint8_t x_tile = 0; x_tile <= 8; ++x_tile)
					{
						uint8_t color = ((pixels1 >> (7 - x_tile)) & 1) | (((pixels2 >> (7 - x_tile)) & 1) << 1);
						int coordX = xPos + x_tile;
						int coordY = yPos + y_tile;
						if (coordX >= 0 && coordX <= 160 && coordY >= 0 && coordY <= 144)
						{
							vertex.append(sf::Vertex(sf::Vector2f(coordX, coordY), colors[color]));

						}
					}
				}
			}
		}
		_window.draw(vertex);
	}
	void LCD::Tick()
	{
		static uint32_t MODE_TIMING[] = { 85, 456, 80, 291 };
		static uint32_t nextTickUpdate = 0;

		Register_Interrupt requestInterrupt = 0;
		if (nextTickUpdate == 0)
		{
			_LYC = _LY;
			uint8_t currentMode = _LCDS.Mode;
			if (currentMode == 2)
			{
				if (_LY >= 153) _LY = 0;
				else
				{
					_LY += 1;
				}
				_LCDS.Mode = 3;
			}
			else if (currentMode == 3)
			{
				_LCDS.Mode = 0;
			}
			else if (currentMode == 0)
			{
				if (_LY >= 144)
				{
					_LCDS.Mode = 1;

					requestInterrupt.VBlank = 1;
					_LCDS.VblankInterruptSource = 1;

					_window.clear({ 0x08, 0x18, 0x28 });
					DrawBackground();
					DrawSprites();
					_window.display();
					sf::Event event;
					while (_window.pollEvent(event)) { }
				}
				else _LCDS.Mode = 2;


			}
			else if (currentMode == 1)
			{
				if (_LY >= 153)
				{
					_LCDS.Mode = 2;
					_LCDS.VblankInterruptSource = 0;
				}
				
				_LY += 1;
			}

			nextTickUpdate = MODE_TIMING[currentMode] / 2;
		}

		if (requestInterrupt > 0) Register_Interrupt::MergeRequestInterrupt(*_mainBus, requestInterrupt);
		nextTickUpdate -= 1;
	}
}