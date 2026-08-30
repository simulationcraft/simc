---
title: Common Issues
---
# Common issues

1. The program crashes near the end of a simulation, right before the report is open. 
> Try adding     decorated_tooltips=0   to your simulation. There are some overzealous "internet security" and VPN programs that crash Simulationcraft during the report generation for currently unknown reasons.
>
> If that does not work, there are various forms of malware that have been crashing Simulationcraft because they infected chrome, which is what Simulationcraft's GUI runs on.
> Try running ADWCleaner, which seems to fix this in a lot of cases - https://toolslib.net/downloads/viewdownload/1-adwcleaner/ - Make sure to restart computer after running this.
 
2. api-ms-win-crt-runtime-l1-1-0.dll is missing!
>  Run windows update and grab everything, or download this - https://support.microsoft.com/en-us/kb/2999226.

3. Simulationcraft randomly blue screens or crashes my computer
>If the computer is blue screening or abruptly restarting during the simulation AND the crashes aren't consistent, it's likely either failing memory, the cpu is not getting enough voltage, or your cpu is overheating due to a cooling system that has somehow broken.
>
>Simc is the most stressful program you can run on a computer outside of cpu benchmarks and prime95, and can expose problems with the stability in your computer that videogames normally do not. 
>
>If you are overclocking, disable the overclock and see if the program works.
>
>Make sure the CPU isn't overheating by using one of the various temperature monitors, speccy, speedfan, etc. 
>
>A guide for testing memory is here - http://www.howtogeek.com/260813/how-to-test-your-computers-ram-for-problems/

4. If you are on Windows 7, make sure to install Service Pack 1. The GUI will crash every time without it.

5. While importing any character: BCP API: Unable to download player from '(Website)' reason: Internal server error.**
> If this happens, unfortunately there is nothing that can be done. It is an internal server error somewhere between blizzard and your computer, and may fix itself with time.

6. Error message when loading program that includes 'Qt(something).dll'
> There have been a few cases that we haven't been able to fix, but a majority of the time this is from antivirus software detecting it as a threat and deleting it. If downloaded from the official links on our site, we can assure you there are no viruses in the program, just tell your antivirus software to ignore it and re-download the file.

7. Intermittent failures on Battle.Net Item downloads
> This issue occurs when fallback download of low level items is required. See [Issue #5278](https://github.com/simulationcraft/simc/issues/5278)

If you are still having issues with the program, please submit an issue ticket, and we will try our best to help you out.
