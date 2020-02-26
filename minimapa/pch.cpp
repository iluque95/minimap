// pch.cpp: el archivo de código fuente correspondiente al encabezado precompilado

#include "pch.h"
#include <cstdint>
#include <Windows.h>
#include <thread>
#include <string>
#include <condition_variable>
#include <sstream>

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

/*

	Public Type Grh

		Frames     As Integer		2
		Frame      As Integer		4
		LastRun    As Long			8
		GrhIndex   As Integer		10
		DelayMS    As Integer		12

	End Type

*/

/*

	Party as byte			0
	PartyNum as byte		2
	Position as Position	4
	Nombre as string		8

*/

/*
	Public Type Position
		x          As Integer
		y          As Integer
	End Type
*/



HDC phdc;
bool run = false;

int32_t VTechoPTR;

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


void CALLBACK CRAW_API_Init(int32_t hwnd, int32_t techo, int32_t MapData, int16_t MDLen, int32_t CharList, int16_t CLLen, int32_t ColorMap, int16_t UserIndex)
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

	VTechoPTR = techo;
}

void Draw_Point(uint8_t x, uint8_t y, COLORREF color)
{
	SetPixel(phdc, x, y, color);
	SetPixel(phdc, x + 1, y, color);
	SetPixel(phdc, x - 1, y, color);
	SetPixel(phdc, x, y - 1, color);
	SetPixel(phdc, x, y + 1, color);
}


std::string getGuild(std::string& name)
{

	return name.substr(name.find('<'), name.length());
}

std::string getName(std::string& name)
{

	return name.substr(0, name.find('<'));
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

					pos += sizeof(int16_t) + 10; // Overpass "blocked" from struct and move to GrhIndex

					auto ptr = reinterpret_cast<int16_t*>(pos);

					int16_t grhindex1 = *(ptr);

					if (grhindex1)
					{
						auto offset = (grhindex1 - 1) * sizeof(int32_t);

						auto color_pos = ColorMapPTR + offset;

						auto color = reinterpret_cast<int32_t*>(color_pos);

						SetPixel(phdc, i + 1, j + 1, *(color));
					}

					auto VTecho = reinterpret_cast<uint8_t*>(VTechoPTR);

					if (*VTecho)
					{

						for (auto k = 2U; k <= 4; ++k)
						{
							int16_t grhindex = *(ptr+(sizeof(int16_t)*k)+10);

							if (grhindex)
							{
								auto offset = (grhindex - 1) * sizeof(int32_t);

								auto color_pos = ColorMapPTR + offset;

								auto color = reinterpret_cast<int32_t*>(color_pos);

								SetPixel(phdc, i + 1, j + 1, *(color));
							}
						}
					}
				}
			}

			// DRAW USERS POINT

			for (auto i = 0U; i < 10000; ++i)
			{
				auto pos = CharListPTR + (i* CharListSize);

				auto ptr = reinterpret_cast<int16_t*>(pos);

				if (pos != UserCharIndex)
				{
					auto party = reinterpret_cast<int16_t*>(pos);
					auto party_num = reinterpret_cast<int16_t*>(pos + 2); // sizeof(int16_t)
					auto x = reinterpret_cast<int16_t*>(pos + 4); 
					auto y = reinterpret_cast<int16_t*>(pos + 6);
					auto guild = reinterpret_cast<char*>(pos + 8);

					std::stringstream ssstream;
					ssstream << guild;
					std::string str = ssstream.str();
					

					auto user_party = reinterpret_cast<int16_t*>(UserCharIndex);
					auto user_party_num = reinterpret_cast<int16_t*>(UserCharIndex + 2);
					auto user_guild = reinterpret_cast<char *>(UserCharIndex + 8);

					std::stringstream user_stream;
					user_stream << user_guild;
					std::string str_user = user_stream.str();

					if (getGuild(str) == getGuild(str_user))
					{
						Draw_Point(*x, *y, RGB(255, 236, 51));
					}
					else if (*party > 0 && *party_num == *user_party_num)
					{
						Draw_Point(*x, *y, RGB(221, 51, 255));
					}
				}
				else
				{
					auto user_x = reinterpret_cast<int16_t*>(UserCharIndex + 4);
					auto user_y = reinterpret_cast<int16_t*>(UserCharIndex + 6);

					Draw_Point(*user_x, *user_y, RGB(255, 0, 0));
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