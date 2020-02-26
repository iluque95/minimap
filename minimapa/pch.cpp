// pch.cpp: el archivo de código fuente correspondiente al encabezado precompilado

#include "pch.h"
#include <cstdint>
#include <Windows.h>
#include <thread>
#include <string>
#include <condition_variable>

// Cuando se utilizan encabezados precompilados, se requiere este archivo de código fuente para que la compilación se realice correctamente.

/*
Public Type tMapBlock
	Blocked    As Byte

	Graphic(1 To 4) As Grh
	CharIndex  As Integer
	ObjGrh     As Grh

	NPCIndex   As Integer
	OBJInfo    As Obj
	TileExit   As WorldPos

	Trigger    As Integer
End Type

*/



HDC phdc;
bool run = false;

bool* VTecho = false;

int32_t MapDataPTR;
uint16_t MapDataSize;

int32_t CharListPTR;
uint16_t CharListSize;

uint8_t framespersecond;

int32_t ColorMapPTR;

int32_t UserCharIndex;


// Thread synchronization

std::condition_variable cv_;
std::mutex m_;
bool data_is_ready_{};


void CALLBACK CRAW_API_Init(int32_t hwnd, int32_t MapData, int16_t MDLen, int32_t CharList, int16_t CLLen, int32_t ColorMap, int16_t UserIndex)
{
	phdc = GetDC((HWND)hwnd);

	MapDataPTR = MapData;
	MapDataSize = MDLen;

	CharListPTR = CharList;
	CharListSize = CLLen;

	//framespersecond = 1000 / fps;

	ColorMapPTR = ColorMap;

	uint32_t offset = (UserIndex - 1) * CLLen;

	UserCharIndex = CharListPTR + offset;
}

void Draw_Point(uint8_t x, uint8_t y, COLORREF color)
{
	SetPixel(phdc, x, y, color);
	SetPixel(phdc, x + 1, y, color);
	SetPixel(phdc, x - 1, y, color);
	SetPixel(phdc, x, y - 1, color);
	SetPixel(phdc, x, y + 1, color);
}

void Render_Minimap()
{
	while (run)
	{

		std::unique_lock<decltype(m_)> l(m_);
		cv_.wait(l, [] { return data_is_ready_; });

		if (run)
		{
			for (auto i = 0U; i < 100; ++i)
			{
				for (auto j = 0U; j < 100; ++j)
				{

					auto pos = MapDataPTR + (i)* MapDataSize + (j) * 100;

					++pos; // Overtake "blocked" from struct.

					auto ptr = reinterpret_cast<int16_t*>(pos);

					int16_t grhindex1 = *(ptr);

					if (grhindex1)
					{
						auto offset = (grhindex1 - 1) * sizeof(int32_t);

						auto color_pos = ColorMapPTR + offset;

						auto color = reinterpret_cast<int32_t*>(color_pos);

						SetPixel(phdc, i + 1, j + 1, *(color));
					}

					if (VTecho)
					{
						int16_t grhindex2 = *(ptr + 2);

						if (grhindex2)
						{
							auto offset = (grhindex2 - 1) * sizeof(int32_t);

							auto color_pos = ColorMapPTR + offset;

							auto color = reinterpret_cast<int32_t*>(color_pos);

							SetPixel(phdc, i + 1, j + 1, *(color));
						}

						int16_t grhindex3 = *(ptr + 4);

						if (grhindex3)
						{
							auto offset = (grhindex3 - 1) * sizeof(int32_t);

							auto color_pos = ColorMapPTR + offset;

							auto color = reinterpret_cast<int32_t*>(color_pos);

							SetPixel(phdc, i + 1, j + 1, *(color));
						}

						int16_t grhindex4 = *(ptr + 6);

						if (grhindex4)
						{
							auto offset = (grhindex4 - 1) * sizeof(int32_t);

							auto color_pos = ColorMapPTR + offset;

							auto color = reinterpret_cast<int32_t*>(color_pos);

							SetPixel(phdc, i + 1, j + 1, *(color));
						}
					}
				}
			}

			// DRAW USER POINT

			for (auto i = 0U; i < 10000; ++i)
			{
				auto pos = CharListPTR + i;

				auto ptr = reinterpret_cast<int16_t*>(pos);

				if (pos != UserCharIndex)
				{
					auto guild = pos;
					auto party = pos;

					auto user_guild = UserCharIndex + 10; // FIX OFFSETS
					auto user_party = UserCharIndex + 15;


					if (guild = user_guild)
					{

						auto x = 100;
						auto y = 200;

						Draw_Point(x, y, RGB(255, 236, 51));
					}
					else if (party = user_party)
					{
						auto x = 100;
						auto y = 200;

						Draw_Point(x, y, RGB(221, 51, 255));
					}
				}
			}

			//std::this_thread::sleep_for(std::chrono::milliseconds(framespersecond));

			data_is_ready_ = false;
		}

	}
}

void CALLBACK CRAW_API_Start()
{
	run = true;
	data_is_ready_ = false;
	std::thread t(Render_Minimap);
	t.detach();
}

void CALLBACK CRAW_API_Draw()
{
	data_is_ready_ = true;
	cv_.notify_one();
}

void CALLBACK CRAW_API_Stop()
{
	run = false;
	CRAW_API_Draw();
}