---
title: Battle Armory API
---
# Introduction

Blizzard will no longer support non-authenticated armory access. We add a default API key for the official binary builds of Simulationcraft, so that users will be able to access their armory without registering to get a key.

However, we still encourage people who make heavy use of Simulationcraft to get their own key. Most users do not fall under this category. Unless you regularly import 20+ unique armories at the same time, you probably do not need to get your own key. In fact, if you only use Simulationcraft to import your own characters, you definitely do not need to get your own key.

People building their own binaries of SimulationCraft are required to obtain their own API key.

The restrictions we are dealing with using the same key for everyone:
  1. Limit of 100 calls per second
  1. Limit of 36,000 calls per hour

#2 will most likely not be an issue, #1 is the one that we are concerned about. We have already taken measures to mitigate #1, such as adding in throttling of api requests when importing more than 1 person in a simulation.

Instead of allowing the client to grab characters as quickly as it can, we have added in a 0.25 second throttle in-between each character. Without this throttle, a single guild importation can easily use 30-50 calls per second. This type of burst traffic will lead to errors and activation of fallbacks while doing the guild import, as there will be other people using simcraft around the world at the same time.

This 0.25 second throttle is disabled if you use your own api key.

We have fallbacks for cases where the call limit is reached, such as scraping html from the armory itself. This method is not as accurate and no where near as fast as using a normal armory lookup, so it is a last resort that hopefully is never used.

**Starting from Simulationcraft 810-02, the following instructions detail how to create client credentials for the Simulationcraft client**

Client credentials can be obtained by going to the <a href='https://develop.battle.net/'>Battle.Net Dev Website.</a>

Registering a key is incredibly easy.
  1. Sign in with a Battle.net account with two-factor authentication enabled
  1. Click "API ACCESS" at the top-left of the page
  1. Click on the "Create new client"
  1. **The only required field on the key registration is the name of the application, choose a globally unique name**
  1. Click create, it will take you to the credential information page
  1. Copy/Paste the 32-character client id into Simulationcraft GUI under "Options" "Globals" "Advanced Settings"
  1. Copy/Paste the 32-character client secret into Simulationcraft GUI under "Options" "Globals" "Advanced Settings"

Note, if you use the command line client, you can put your client credentials into a file called "./apikey.txt" that is located in the same directory as the simulationcraft command line executable. On unixes (anywhere that has the HOME environment variable defined), you can also place the client credentials into a file called ".simc\_apikey" in your home directory. The format of the file is a single line, with client id and secret separated by a colon (:) character (for example `0123456789012345678901:0123456789012345678901`).

**WinXP will likely NOT work with the new armory API, even if you use a key. The SSL library on XP doesn't seem to be compatible with the code we are using.**

