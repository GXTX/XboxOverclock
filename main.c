/*
	xbox_oc
	----------------
	Copyright (C) 2022 wutno (https://github.com/GXTX)
	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.
	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.
	You should have received a copy of the GNU General Public License along
	with this program; if not, write to the Free Software Foundation, Inc.,
	51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include <hal/debug.h>
#include <hal/video.h>
#include <hal/xbox.h>
#include <windows.h>
#include <SDL.h>

#define BASE_CLOCK_INT   16667
#define BASE_CLOCK_FLOAT 16.667f

// https://github.com/WulfyStylez/XBOverclock
void calc_clock_params(int clk, int *n, int *m)
{
	int work1, work2, work4;

	work1 = clk / BASE_CLOCK_FLOAT;
	work2 = (clk * 2) / BASE_CLOCK_FLOAT;
	work4 = (clk * 4) / BASE_CLOCK_FLOAT;

	if (work2 * 2 != work4)
	{
		*n = work4;
		*m = 4;
	}
	else if (work1 * 2 != work2)
	{
		*n = work2;
		*m = 2;
	}
	else
	{
		*n = work1;
		*m = 1;
	}
}

int main(void)
{
	XVideoSetMode(640, 480, 32, REFRESH_DEFAULT);

	SDL_Init(SDL_INIT_GAMECONTROLLER);
	SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");

	SDL_Event event;
	SDL_GameController *controller;

	// We assume the user had a controller plugged into port 1
	controller = SDL_GameControllerOpen(0);

	// Read in the current FSB setting
	ULONG pci_buff;
	ULONG pci_addr = 0x8000036C;
	WRITE_PORT_BUFFER_ULONG((PULONG)0xCF8, &pci_addr, 1);
	READ_PORT_BUFFER_ULONG((PULONG)0xCFC, &pci_buff, 1);

	ULONG wanted_fsb = (int)((pci_buff >> 8) & 0xFF) * BASE_CLOCK_FLOAT;

	// There is a bug in the calculation where we can sometimes display way outside the acceptable range
	if (wanted_fsb > 200 || wanted_fsb < 100) {
		wanted_fsb = 133;
	}

	ULONG current_nvclk = *((ULONG *)0xFD680500);
	ULONG wanted_nvclk = (((BASE_CLOCK_INT * ((current_nvclk & 0xFF00) >> 8)) / (1 << ((current_nvclk & 0x70000) >> 16))) / (current_nvclk & 0xFF)) / 1000;

	debugPrint("FSB   : %dMHz\n", wanted_fsb);
	debugPrint("NVCLK : %dMHz\n", wanted_nvclk);

	debugPrint("\nThis tool may cause irreparable harm to your Xbox.\n");
	debugPrint("This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n");
	debugPrint("Applying, the machine should return to your dashboard, if you do not see \"SET\" the box has frozen and you should reboot and try again.\n");
	debugPrint("\n==============================\n");
	debugPrint("Use the left and right \"DPAD\" to change the FSB.\n");
	debugPrint("Use the up and down \"DPAD\" to change the NVCLK.\n");
	debugPrint("Press \"Start\" to apply (which will auto reboot).\n");
	debugPrint("Press \"Back\" to exit.");
	debugResetCursor();

	while (1)
	{
		while (SDL_PollEvent(&event))
		{
			if (event.type == SDL_CONTROLLERBUTTONDOWN)
			{
				switch (event.cbutton.button)
				{
				case SDL_CONTROLLER_BUTTON_DPAD_LEFT: wanted_fsb--; break;
				case SDL_CONTROLLER_BUTTON_DPAD_RIGHT: wanted_fsb++; break;
				case SDL_CONTROLLER_BUTTON_DPAD_DOWN: wanted_nvclk--; break;
				case SDL_CONTROLLER_BUTTON_DPAD_UP: wanted_nvclk++; break;
				case SDL_CONTROLLER_BUTTON_START:
					SDL_Quit(); // have less stuff running

					int n, m;

					if (wanted_nvclk != 233) {
						calc_clock_params(wanted_nvclk * 2, &n, &m);
						debugClearScreen();
						debugPrint("Setting NVCLK to: %dMHz\n", (BASE_CLOCK_INT * n / m) / 2 / 1000);

						ULONG coeff = (current_nvclk & ~0x0000FFFF) | (n << 8) | m;

						Sleep(500);
						*((ULONG *)0xFD680500) = coeff;
						Sleep(500);
					}

					if (wanted_fsb != 133) {
						calc_clock_params(wanted_fsb, &n, &m);
						int clk = BASE_CLOCK_FLOAT * n / m;
						debugClearScreen();
						debugPrint("Setting FSB to: %dMHz\n", clk);
						debugPrint("CPU: %dMHz\n", (int)(clk * 5.5f));

						ULONG coeff = (pci_buff & ~0x0000FFFF) | (n << 8) | m;

						// wait
						Sleep(500);
						asm __volatile__("cli"); // disable interrupts
						Sleep(500);
						asm("nop");
						asm("nop");
						asm("nop");
						asm("nop");
						asm("nop");

						WRITE_PORT_BUFFER_ULONG((PULONG)0xCF8, &pci_addr, 1);
						WRITE_PORT_BUFFER_ULONG((PULONG)0xCFC, &coeff, 1);

						// wait
						asm("nop");
						asm("nop");
						asm("nop");
						asm("nop");
						asm("nop");
						asm("nop");
						Sleep(500);
						asm __volatile__("sti"); //enable interrupts
						Sleep(500);
					}

					debugPrint("\nSET\n");

					goto the_end;
					break; // unreachable
				case SDL_CONTROLLER_BUTTON_BACK:
					goto the_end;
					break; // unreachable
				default:
					break;
				}
				
				debugResetCursor();
				debugPrint("FSB   : %dMHz\n", wanted_fsb);
				debugPrint("NVCLK : %dMHz\n", wanted_nvclk);
			}
		}
	}

the_end:
	SDL_Quit();
	return 0;
}
