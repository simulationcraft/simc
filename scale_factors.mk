# ==========================================================================
# Dedmonwakeen's Raid DPS/TPS Simulator.
# Send questions to natehieter@gmail.com
# ==========================================================================

# Ugh

profile = $(subst death_knight,Death_Knight,$(subst dr,Dr,$(subst hu,Hu,$(subst mag,Mag,$(subst p,P,$(subst ro,Ro,$(subst sh,Sh,$(subst wa,Wa,$1))))))))

# Regenerate html reports when class models change

MODULE = ./simc
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

SRC        = engine
DEPENDS    = $(SRC)/simulationcraft.h
ITERATIONS = 25000
THREADS    = 2
SF         = 1
OPTS       = iterations=$(ITERATIONS) threads=$(THREADS) calculate_scale_factors=$(SF) hosted_html=1
GEAR       = T11_372

.PHONY:	live ptr all clean

live: $(REPORTS_LIVE)

ptr: $(REPORTS_PTR)

all: live ptr

clean:
	/bin/rm -f $(REPORTS_LIVE) $(REPORTS_PTR)

$(HTML)/$(LIVE)/%.html: $(SRC)/sc_%.cpp $(DEPENDS)
	-@echo Generating $@ 
	-@$(MODULE) $(OPTS) $(call profile,$(basename $(@F)))_$(GEAR).simc output=$(basename $(@F)).txt html=$@

# We should make <class>_PTR_<etc>.simc files even if they just reload the live version

$(HTML)/$(PTR)/%.html: $(SRC)/sc_%.cpp $(DEPENDS)
	-@echo Generating $@ 
	-@$(MODULE) ptr=1 $(OPTS) $(call profile,$(basename $(@F)))_$(GEAR)_PTR.simc output=$(basename $(@F)).txt html=$@

