# ==========================================================================
# Dedmonwakeen's Raid DPS/TPS Simulator.
# Send questions to natehieter@gmail.com
# ==========================================================================

# Regenerate html reports when class models change

MODULE = simc
HTML   = html
LIVE   = 406
PTR    = 410

MODELS =\
	death_knight	\
	druid		\
	hunter		\
	mage		\
	paladin		\
	priest		\
	rogue		\
	shaman		\
	warlock		\
	warrior 

REPORTS_LIVE := $(MODELS:%=$(HTML)/$(LIVE)/%.html)
REPORTS_PTR  := $(MODELS:%=$(HTML)/$(PTR)/%.html)

.PHONY:	live ptr all

live: $(REPORTS_LIVE)

ptr: $(REPORTS_PTR)

all: live ptr

$(HTML)/$(LIVE)/%.html: engine/sc_%.cpp
	-@echo Generating $@

$(HTML)/$(PTR)/%.html: engine/sc_%.cpp
	-@echo Generating $@
