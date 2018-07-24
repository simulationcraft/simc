
# How Can I Contribute?

The SimulationCraft team is comprised of volunteer developers from all over the world. We are always looking for new contributors.
While C++ expertise is certainly helpful, we have several key members with limited coding experience.
Maintaining optimal talent, gear, and default action priority lists is a huge task and requires virtually zero C++ knowledge.

We are a very laid back group of developers. While certain project members have areas of expertise, 
there is little in the way of strict responsibility and ownership. Developers are expected to exercise their 
initiative and help out wherever needed. GitHub provides considerable utilities for oversight. 
The mantra is: Just check it in. Don't ask for permission. If the code needs to be changed for functional (or artistic!) reasons,
someone will revert/modify as needed.

## Profiles
SimulationCraft includes a set of sample profiles for the current raiding tier, which are located in /profiles.
If you want to improve those profiles, please note to the following:
 * Change character profile settings like talents & gear in the files you can find at /profiles/generators.
   The files in /profiles are auto-generated and will be overriden.

## Action Priority List (APL)
SimulationCraft includes a set of default action priority list for each specialization, to offer users a ready-to-use 
simulation experience once they import their character. This APL is the main factor defining if a character performs just ok 
or as good as possible, maximizing their DPS. Naturally, these APL can be constantly improved.

You can directly modify and improve a existing set of action lists and contribute that either directly to your classes main 
maintainer, through your class Discord or directly as a ticket on Github.

 * As a developer or experienced user, please try to port Action Priority List (APL) changes directly to the class module code,
   eg. /engine/class_modules/sc_mage.cpp . You can usually find the apl functions near the bottom of the file in functions 
   called. eg. mage_t::apl_fire()

# Join the Team

If you are interested in joining the team, contact us on Discord or send an email to natehieter@gmail.com with your contact info.
We look forward to working with you!

Discord: [SimCMinMax](https://discord.gg/tFR2uvK) (#simulationcraft)

# Developers Resources
Checkout the [Developers Corner wiki page](../../wiki/Participate) for more detailed information on how to setup your 
SimulationCraft development environment.

Platform specific building instructions can be found on the [How-To-Build wiki](../../wiki/HowToBuild).

Please check out our [CodingGuidelines](../../wiki/CodingGuidelines) before you start.
