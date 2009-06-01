// internal debug flag, only for this unit
// 0=off
// 1=regular debug
// 2=more detailed debug, each item stat shown
// 3=each URL read shown
static int debug=0;


#include "simcraft.h"
using namespace std;

const bool ParseEachItem=true;

struct urlSplit_t
{
  std::string wwwAdr;
  std::string realm;
  std::string player;
  int srvType;
  sim_t* sim;
  std::string generated_options;
  unsigned int setPieces[20];
};

enum url_page_t {  UPG_GEAR, UPG_TALENTS, UPG_ITEM  };

std::string tolower( std::string src )
{
  std::string dest=src;
  for ( unsigned i=0; i<dest.length(); i++ )
    dest[i]=tolower( dest[i] );
  return dest;
}

void replace_char( std::string& src, char old_c, char new_c  )
{
  for (int i=0; i<(int)src.length(); i++)
    if (src[i]==old_c)
      src[i]=new_c;
}


//wrapper supporting differnt armory pages
std::string getArmoryData( urlSplit_t aURL, url_page_t pgt, std::string moreParams="" )
{
  std::string URL=aURL.wwwAdr;
  std::string chk="";
  int throttle=aURL.sim->http_throttle;
  switch ( pgt )
  {
    case UPG_TALENTS:   
      URL+= "/character-talents.xml";   
      chk="</talentGroup>";   
      break;
    case UPG_ITEM:      
      URL+= "/item-tooltip.xml";        
      chk="</itemTooltip>";   
      break;
    case UPG_GEAR:      
    default:            
      URL+= "/character-sheet.xml";     
      chk="</characterTab>";  
      throttle+=1;
      break;
  }
  URL+="?r="+aURL.realm+"&n="+aURL.player+moreParams;
  std::string result;
  if( ! http_t::get( result, URL, chk, throttle ) ) result = "";
  return result;
}



// convert full descriptive name into "option name"
// spaces become underscores, all lower letters
// remove apostrophes
std::string proper_option_name( std::string& full_name )
{
  if ( full_name=="" ) return full_name;
  // first to lower letters and _ for spaces
  std::string newName( full_name.length(),' ' );
  newName="";
  for ( size_t i=0; i<full_name.length(); i++ )
  {
    char c=full_name[i];
    c= tolower( c );		// lower case
    if ( c==' ' ) c='_';	// spaces to underscores
    if ( c!='\'' )      // remove apostrophes
      newName+=c;
  }
  // then remove first "of" for glyphs (legacy)
  if ( newName.substr( 0,9 )=="glyph_of_" )   newName.erase( 6,3 );
  // return result
  return newName;
}





//split URL to server, realm, player
bool splitURL( std::string URL, urlSplit_t& aURL )
{
  size_t iofs=0;
  size_t id_http= URL.find( "http://",iofs );
  if ( id_http != string::npos ) iofs=7;
  size_t id_folder= URL.find( "/", iofs );
  if ( id_folder != string::npos ) iofs=id_folder+1;
  size_t id_srv= URL.find( "?r=",iofs );
  if ( id_srv != string::npos ) iofs=id_srv+1;
  size_t id_name= URL.find( "&cn=",iofs );
  size_t id_name_sz=4;
  if ( string::npos == id_name )
  {
    id_name= URL.find( "&n=",iofs );
    id_name_sz=3;
  }
  aURL.wwwAdr="";
  aURL.realm="";
  aURL.player="";
  aURL.srvType=1;
  aURL.generated_options="";
  if ( id_name != string::npos )
  {
    //extract url, realm and player names
    aURL.wwwAdr=URL.substr( 0,id_folder );
    aURL.realm=URL.substr( id_srv+3,id_name-id_srv-3 );
    size_t id_amp=URL.find( "&",id_name+1 );
    if ( id_amp != string::npos )
      aURL.player=URL.substr( id_name+id_name_sz,id_amp-id_name-id_name_sz );
    else
      aURL.player=URL.substr( id_name+id_name_sz,URL.length()-id_name-id_name_sz );
  }
  else
    return false;
  // post proccessing
  if (aURL.wwwAdr.length()<=3) aURL.wwwAdr+=".wowarmory.com";
  if ( string::npos == id_http ) aURL.wwwAdr="http://"+aURL.wwwAdr;

  for ( unsigned int i=0; i<20; i++ )  aURL.setPieces[i]=0;

  return true;
}



// retrieve node from XML
// every node must start with <name , and then there are options:
// <name> xxxx </name> or  <name  a,b,c /> or  <name/>
std::string getNodeOne( std::string& src, std::string name, int occurence=1 )
{
  std::string nstart="<"+name;
  //fint n-th occurence of node
  size_t offset=0;
  size_t idx;
  do
  {
    std::string allowedNext=" >/";
    idx=src.find( nstart,offset );
    bool found=false;
    while ( ( idx != string::npos )&&( !found ) )
    {
      char n=src[idx+nstart.length()];
      size_t i2=allowedNext.find( n );
      if ( i2 != string::npos )
        found=true;
      else
        idx=src.find( nstart,idx+1 );
    }
    occurence--;
    offset=idx+1;
  }
  while ( ( idx != string::npos )&&( occurence>0 ) );
  // if node start found, find end
  if ( ( idx != string::npos )&&( occurence==0 ) )
  {
    size_t np=nstart.length();
    std::string nextChar=src.substr( idx+np,1 );
    size_t idxEnd;
    bool singleLine=true;
    if ( nextChar==">" )
      singleLine=false;
    else
    {
      size_t idLn= src.find( "\n",idx+1 );
      size_t idEnd=src.find( "/>",idx+1 );
      if ( ( idEnd == string::npos )||( idEnd != string::npos && idEnd>idLn ) ) singleLine=false;
    }
    if ( singleLine )
      nstart="/>"; // for single line nodes, it ends with />  ..and presume NO inline nodes
    else
      nstart="</"+name+">"; // if this is multiline node, it ends with </name>
    idxEnd=src.find( nstart,idx+1 );
    size_t i1=idx+name.length()+2;
    if ( idxEnd != string::npos && idxEnd>i1 )
    {
      std::string res= src.substr( i1,idxEnd-i1 );
      return res;
    }
  }
  return "";
}




// allow node path to get subnodes, from nodeList= "node1.node2.node3"
std::string getNode( std::string& src, std::string nodeList )
{
  std::string res=src;
  std::string node2;
  size_t last_find=0;
  size_t idx= nodeList.find( ".",last_find );
  //get all nodes before last
  while ( idx != string::npos && idx>last_find )
  {
    node2= nodeList.substr( last_find,idx-last_find );
    res=getNodeOne( res, node2 );
    last_find=idx+1;
    idx= nodeList.find( ".",last_find );
  }
  //get last node
  node2= nodeList.substr( last_find,nodeList.length()-last_find );
  res=getNodeOne( res, node2 );
  return res;
}


// retrieve string value from XML single line node.
// example: increasedHitPercent="12.47" penetration="0" reducedResist="0" value="327"
std::string getValueOne( std::string& node, std::string name )
{
  std::string res="";
  std::string src=" "+node;
  std::string nstart=" "+name+"=\"";
  size_t idx=src.find( nstart );
  if ( idx != string::npos )
  {
    size_t i1=idx+nstart.length();
    size_t idxEnd=src.find( "\"",i1 );
    if ( idxEnd != string::npos && idxEnd>i1 )
    {
      res= src.substr( i1,idxEnd-i1 );
    }
  }
  return res;
}

//return parameter value, given path to parameter
// example:  spell.bonusDamage.petBonus.damage
// where last is name of parameter
std::string getValue( std::string& src, std::string path )
{
  //find parameter name (last)
  size_t idx=path.rfind( "." );
  if ( idx != string::npos && idx>0 )
  {
    // get param name
    std::string paramName= path.substr( idx+1,path.length()-idx );
    //get subnode where param is
    path=path.substr( 0,idx );
    std::string subnode= getNode( src, path );
    //return param
    return getValueOne( subnode, paramName );
  }
  else
    // if only parameter name is given, no dots, for example "damage"
    return getValueOne( src,path );
}


// get max value out of list of paths, separated by ,
//example:  "spell.power,melee.power"
std::string getMaxValue( std::string& src, std::string path )
{
  std::string res="";
  double best=0;
  std::vector<std::string> paths;
  unsigned int num_paths = util_t::string_split( paths,path, "," );
  for ( unsigned int i=0; i < num_paths; i++ )
  {
    std::string res2=getValue( src, paths[i] );
    if ( res2!="" )
    {
      double val=atof( res2.c_str() );
      if ( ( res=="" )||( val>best ) )
      {
        best=val;
        res=res2;
      }
    }
  }
  return res;
}

//get float value from param inside node
double getParamFloat( std::string& src, std::string path )
{
  std::string res= getValue( src, path );
  return atof( res.c_str() );
}

//get float value from entire node
double getNodeFloat( std::string& src, std::string path )
{
  std::string res= getNode( src, path );
  size_t idB= res.find( ">" ); // for <armor armorBonus="0">155</armor>
  if ( ( idB != string::npos )&&( idB<( res.length()-1 ) ) )
    res.erase( 0,idB+1 );
  return atof( res.c_str() );
}


std::string chkValue( std::string& src, std::string path, std::string option )
{
  std::string res=getValue( src, path );
  if ( res!="" )
    return option+res+"\n";
  else
    return "";
}

std::string chkMaxValue( std::string& src, std::string path, std::string option )
{
  std::string res=getMaxValue( src, path );
  if ( res!="" )
    return option+res+"\n";
  else
    return "";
}


unsigned int  getSetTier( std::string setName );




// call players parse option
bool player_parse_option( urlSplit_t& aURL, const std::string& name, const std::string& value )
{
  if ( !aURL.sim->active_player ) return false;
  bool ok=aURL.sim->active_player->parse_option( name,value );
  if (ok){
    std::string opt_str= name +"="+ value +"\n";
    aURL.generated_options+=opt_str;
    if ( debug ) printf("%s",opt_str.c_str() );
  }
  return ok;
}

// call player_parse_option for every item, if special option exists
// useful only for GEMs and ENCHANTs
// since for items and glyphs "proper names" are generated and tried as options
void addItemGlyphOption( urlSplit_t& aURL, std::string&  node, std::string name )
{
  if ( node!="" )
  {
    std::string itemID= getValue( node, name );
    if ( itemID !="" )
    {
      //rename few well known gems, to be more readable
      if (itemID=="41285") itemID="chaotic_skyflare"; else
      if (itemID=="41333") itemID="ember_skyflare"; else
      itemID="item_"+itemID;
      //call parse to check if this item option exists
      player_parse_option( aURL,itemID,"1" );
    }
  }
}




// display all stats
void displayStats( gear_stats_t& gs, gear_stats_t* gsDiff=0 )
{
  if ( gsDiff ) printf( "Gear Stats Difference:\n" ); else printf( "Gear Stats:\n" );
  for ( unsigned int i=STAT_NONE; i<STAT_MAX; i++ )
  {
    double oldVal=0;
    if ( gsDiff ) oldVal=gsDiff->get_stat( i );
    double val=gs.get_stat( i )- oldVal;
    if ( val!=0 )
      printf( "    %s = %lf\n",util_t::stat_type_string( i ), val );
  }
}



// adds option for set bonuses
void  addSetInfo( urlSplit_t& aURL, std::string setName, unsigned int setPieces,std::string  item_id )
{
  if ( !aURL.sim->active_player ) return;
  unsigned int tier= getSetTier( setName );
  // set tier option if any
  if ( (tier>0)&& (setPieces>aURL.setPieces[tier]) )
  {
    char setOption[100];
    std::string strOption, strValue;
    strValue="1";
    if ( ( setPieces>=2 )&&( aURL.setPieces[tier]<2 ) )
    {
      sprintf( setOption,"tier%d_2pc",tier );
      strOption=setOption;
      player_parse_option( aURL,strOption,strValue );
    }
    if ( ( setPieces>=4 )&&( aURL.setPieces[tier]<4 ) )
    {
      sprintf( setOption,"tier%d_4pc",tier );
      strOption=setOption;
      player_parse_option( aURL,strOption,strValue );
    }
    aURL.setPieces[tier]= setPieces;
  }
}



bool my_isdigit( char c )
{
  if ( c=='+' ) return true;
  if ( c=='-' ) return true;
  return isdigit( c )!=0;
}

// find value/pattern pair
double oneTxtStat( std::string& txt, std::string fullpat, int dir, int type  )
{
  size_t idx=0;
  double value=0;
  //support multiple occurences of same pattern
  while ( idx != string::npos )
  {
    idx= txt.find( fullpat );
    if ( idx != string::npos )
    {
      size_t idL=idx;
      size_t idR=idx+fullpat.length();
      std::string strVal="";
      size_t dL=0;
      size_t dR=0;
      if ( dir>0 )
      {
        size_t p=idR;
        while ( ( p<txt.length() )&&( txt[p]==' ' ) ) p++; //skip spaces
        dL=dR=p;
        while ( ( p<txt.length() )&& my_isdigit( txt[p] ) ) p++; //walk over number
        dR=p;
        idR=dR;
      }
      else
      {
        int p=idL-1;
        while ( ( p>=0 )&&( txt[p]==' ' ) ) p--; //skip spaces
        dR=dL=(size_t) (p+1);
        while ( ( p>=0 )&& my_isdigit( txt[p] ) ) p--; //walk over number
        dL=(size_t)(p+1);
        idL=dL;
      }
      //extract value
      if ( dR>dL )
      {
        strVal=txt.substr( dL,dR-dL );
        if ( strVal!="" )
        {
            // if value is on the left, it must have "+" in "Equip:" texts
            bool plusOK= ( dir>0 ) || (type!=2) || ( strVal[0]=='+');
            if  (plusOK)
              value+=atof( strVal.c_str() );
        }
      }
      //delete this instance
      txt.erase( idL,idR-idL );
    }
  }
  return value;
}

// Try to find txt pattern and value pair.  This looks for "+X to All Stats" and adds all the stats
// If found, it will remove the pattern/value from txt
const bool chkAllStatsBonus( gear_stats_t& gs, std::string& txt, int type , const std::string& pattern)
{
  bool ok=false;
  
  std::string pat;
  double val;
  
  pat = " "+pattern;
  val = oneTxtStat( txt, pat, -1, type );
  if( val )
  {
    gs.add_stat( STAT_STRENGTH, val );
    gs.add_stat( STAT_AGILITY, val );
    gs.add_stat( STAT_STAMINA, val );
    gs.add_stat( STAT_INTELLECT, val );
    gs.add_stat( STAT_SPIRIT, val );
    ok = true;
  }
  return false;
}

//try to find txt pattern and value pair. Patterns are expected in lower cases
// it will try first: "pattern by XX", then "+XX pattern"
// if found, it will remove pattern/value from txt
bool chkOneTxtStat( gear_stats_t& gs, std::string& txt, int type , int statID, std::string pattern )
{
  bool ok=false;

  std::string pat;
  double val;
  // this patterns should increase stat
  pat= " improves "+pattern+" by ";
  val= oneTxtStat( txt, pat, +1, type );
  if ( val )
  {
    gs.add_stat( statID,  val );
    ok=true;
  }
  pat= " increases "+pattern+" by ";
  val= oneTxtStat( txt, pat, +1, type );
  if ( val )
  {
    gs.add_stat( statID,  val );
    ok=true;
  }
  // this pattern oncrease stat ONLY if number to the left start with +
  // it STILL has risk to admit some "chance to do +25 spell dmg", but in all texts so far
  // all stat numbers of that type (ie in procs) appear to be without +
  pat= " "+pattern;
  val= oneTxtStat( txt, pat, -1, type );
  if ( val )
  {
    gs.add_stat( statID,  val );
    ok=true;
  }
  return false;
}

// parse text and search for stats to add
// type is source of text, needed to decide when "+" is needed
// type: 1==enchant, 2==spellData(Equip:), 3==gem, 4==socketBonus
void addTextStats( gear_stats_t& gs, std::string txt, int type )
{
  if ( txt=="" ) return;
  txt= " "+tolower( txt )+" ";

  //chkOneTxtStat(gs,txt, STAT_SPELL_POWER,               "shadow spell damage"); //problem: item may have shdw+frost+fire...
  chkAllStatsBonus( gs,txt,type,                              "all stats" );
  chkOneTxtStat( gs,txt,type, STAT_SPELL_POWER,               "spell power" );
  chkOneTxtStat( gs,txt,type, STAT_MP5,                       "mana regen" );
  chkOneTxtStat( gs,txt,type, STAT_MP5,                       "mana every" );
  chkOneTxtStat( gs,txt,type, STAT_MP5,                       "mana per" );
  chkOneTxtStat( gs,txt,type, STAT_MP5,                       "mana restored per" );
  chkOneTxtStat( gs,txt,type, STAT_MP5,                       "mana/5" );
  chkOneTxtStat( gs,txt,type, STAT_ATTACK_POWER,              "attack power" );
  chkOneTxtStat( gs,txt,type, STAT_EXPERTISE_RATING,          "expertise rating" );
  chkOneTxtStat( gs,txt,type, STAT_ARMOR_PENETRATION_RATING,  "armor penetration" );
  chkOneTxtStat( gs,txt,type, STAT_HASTE_RATING,              "haste rating" );
  chkOneTxtStat( gs,txt,type, STAT_HIT_RATING,                "ranged hit rating" ); //we dont have them separate
  chkOneTxtStat( gs,txt,type, STAT_HIT_RATING,                "hit rating" );
  chkOneTxtStat( gs,txt,type, STAT_CRIT_RATING,               "ranged critical strike" );
  chkOneTxtStat( gs,txt,type, STAT_CRIT_RATING,               "critical strike rating" );
  chkOneTxtStat( gs,txt,type, STAT_CRIT_RATING,               "crit rating" );
  chkOneTxtStat( gs,txt,type, STAT_STRENGTH,                  "strength" );
  chkOneTxtStat( gs,txt,type, STAT_AGILITY,                   "agility" );
  chkOneTxtStat( gs,txt,type, STAT_STAMINA,                   "stamina" );
  chkOneTxtStat( gs,txt,type, STAT_INTELLECT,                 "intellect" );
  chkOneTxtStat( gs,txt,type, STAT_SPIRIT,                    "spirit" );
  //shorter, less safe parses
  chkOneTxtStat( gs,txt,type, STAT_ARMOR,                     "armor" );
  chkOneTxtStat( gs,txt,type, STAT_HEALTH,                    "health" );
  chkOneTxtStat( gs,txt,type, STAT_MANA,                      "mana" );

}


// parse stats from one item and adds it to total stats in "gs"
// also should parse weapon stats
bool  parseItemStats( urlSplit_t& aURL, gear_stats_t& gs,  std::string& item_id, std::string& item_slot )
{
  if ( item_id=="" ) return false;
  std::string src= getArmoryData( aURL,UPG_ITEM, "&i="+item_id+"&s="+item_slot );
  if ( src=="" )  return false;
  // add fixed stats
  gs.add_stat( STAT_STRENGTH,                  getNodeFloat( src, "bonusStrength" ) );
  gs.add_stat( STAT_AGILITY,                   getNodeFloat( src, "bonusAgility" ) );
  gs.add_stat( STAT_STAMINA,                   getNodeFloat( src, "bonusStamina" ) );
  gs.add_stat( STAT_INTELLECT,                 getNodeFloat( src, "bonusIntellect" ) );
  gs.add_stat( STAT_SPIRIT,                    getNodeFloat( src, "bonusSpirit" ) );
  gs.add_stat( STAT_SPELL_POWER,               getNodeFloat( src, "bonusSpellPower" ) );
  gs.add_stat( STAT_MP5,                       getNodeFloat( src, "bonusManaRegen" ) );
  gs.add_stat( STAT_ATTACK_POWER,              getNodeFloat( src, "bonusAttackPower" ) );
  gs.add_stat( STAT_EXPERTISE_RATING,          getNodeFloat( src, "bonusExpertiseRating" ) );
  gs.add_stat( STAT_ARMOR_PENETRATION_RATING,  getNodeFloat( src, "bonusArmorPenetration" ) );
  gs.add_stat( STAT_HASTE_RATING,              getNodeFloat( src, "bonusHasteRating" ) );
  gs.add_stat( STAT_HIT_RATING,                getNodeFloat( src, "bonusHitRating" ) );
  gs.add_stat( STAT_CRIT_RATING,               getNodeFloat( src, "bonusCritRating" ) );
  gs.add_stat( STAT_ARMOR,                     getNodeFloat( src, "armor" ) );
  // add textual - descriptive - stats
  addTextStats( gs, getNode( src, "enchant" ),1 );
  addTextStats( gs, getNode( src, "spellData.desc" ),2 );
  // parse gems and sockets
  std::string node= getNode( src, "socketData" );
  bool allMatch=true;
  for ( unsigned int i=1; i<=5; i++ )
  {
    std::string gem= getNodeOne( node, "socket" ,i );
    if ( gem!="" )
    {
      addTextStats( gs, getValue( gem, "enchant" ),3 );
      if ( getValue( gem, "match" )!="1" ) allMatch=false;
    }
  }
  // add socket bonus if all match
  if ( allMatch )
  {
    addTextStats( gs, getNode( node, "socketMatchEnchant" ),4 );
  }
  // parse set bonus
  node= getNode( src, "setData" );
  std::string setName= getNode( node, "name" );
  if ( setName!="" )
  {
    unsigned int setPieces=0;
    for ( unsigned int i=1; i<=6; i++ )
    {
      std::string sItem= getNodeOne( node, "item" ,i );
      if ( getValue( sItem, "equipped" )=="1" ) setPieces++;
    }
    // add info to set list
    addSetInfo( aURL, setName, setPieces, item_id );
  }
  // parse weapon stats if its weapon
  // main_hand=fist,dps=171,speed=2.6,enchant=berserking
  std::string wpnName="";
  if ( item_slot=="15" ) wpnName="main_hand"; else
      if ( item_slot=="16" ) wpnName="off_hand"; else
      if ( item_slot=="17" ) wpnName="ranged";
  if ( wpnName!="" )
  {
    std::string wpnValue= tolower( getNode( src, "equipData.subclassName" ) );
    if ( wpnValue!="" )
    {
      wpnValue+= ",dps="+getNode( src, "damageData.dps" );
      wpnValue+= ",speed="+getNode( src, "damageData.speed" );
      std::string ench=tolower(getNode( src, "enchant" ));
      for (int i=0; i<WEAPON_ENCHANT_MAX; i++){
        std::string reg_ench=util_t::weapon_enchant_type_string(i);
        if (ench.find(reg_ench)!= string::npos){
          wpnValue+= ",enchant="+reg_ench;
          break;
        }
      }
      player_parse_option( aURL,wpnName,wpnValue );
    }
  }
  // check if there is any option for this item
  std::string item_name=getNode( src, "itemTooltip.name" );
  if ( item_name!="" )
    player_parse_option( aURL, proper_option_name( item_name ),"1" );

  return true;
}


// copy gear stats toplayer and form option str
bool  copy_gear_to_player(urlSplit_t& aURL, gear_stats_t& gs){
  if (!aURL.sim->active_player) return false;
  aURL.sim->active_player->gear_stats = gs;
  //form option string
  std::string opt="";
  for ( unsigned int i=STAT_NONE; i<STAT_MAX; i++ )
  {
    double val=gs.get_stat( i );
    if ( val!=0 ){
       char buffer[1000];
       sprintf(buffer, "gear_%s=%1.0lf\n",util_t::stat_type_string( i ), val );
       opt=buffer;
       if (debug) printf(opt.c_str());
       aURL.generated_options+=opt;
    }
  }

  return true;
}


// save simcraft file based on parsed player
bool save_player_simcraft(urlSplit_t& aURL)
{
  thread_t::lock();
  std::string file_name="armory_"+aURL.player+".simcraft";
  FILE* file = fopen( file_name.c_str(), "w" );
  if( file )
  {
    fwrite( aURL.generated_options.c_str(), aURL.generated_options.length()+1, 1, file );
    fclose( file );
  }
  thread_t::unlock();
  return true;
}




// main procedure that parse armory info from giuven player URL (URL must have realm/player inside)
// it set options either by building option string and calling parse_line(), or by directly calling
// player->parse_option()
bool parseArmory( sim_t* sim, std::string URL, bool inactiveTalents=false, bool gearOnly=false, bool save_player=false )
{
  std::string optionStr="";
  // split URL
  urlSplit_t aURL;
  if ( !splitURL( URL,aURL ) ){
    printf("\nArmory parse wrong URL: %s\n",URL.c_str());
    return false;
  }
  aURL.sim=sim;
  debug=sim->debug_armory;
  // retrieve armory gear data (XML) for this player
  std::string src= getArmoryData( aURL,UPG_GEAR );
  if ( src=="" ){
    printf("\nArmory parse incorrect page for: \narmory=%s , realm=%s , player=%s\n",
           aURL.wwwAdr.c_str(), aURL.realm.c_str(),aURL.player.c_str());
    return false;
  }

  // parse that XML and search for available data
  // each recognized option add to optionStr
  std::string node;
  node= getValue( src,"character.class" );
  if (node==""){
    printf("\nNo <character.class> in Armory parse !\n");
    if (debug) printf("Src=%s\n",src.c_str());
    return true;
  }
  std::string charName= getValue(src,"character.name");
  if (inactiveTalents) charName+="_inactive";
  if ( (!gearOnly)&&( node!="" ) )
  {
    node[0]=tolower( node[0] );
    optionStr+= node+"="+charName+"\n";
    optionStr+= chkValue( src,"character.level","level=" );
  }

  //gear stats from summary tabs
  if ( !ParseEachItem )
  {
    optionStr+= chkValue( src,"baseStats.strength.effective","gear_strength=" );
    optionStr+= chkValue( src,"baseStats.agility.effective","gear_agility=" );
    optionStr+= chkValue( src,"baseStats.stamina.effective","gear_stamina=" );
    optionStr+= chkValue( src,"baseStats.intellect.effective","gear_intellect=" );
    optionStr+= chkValue( src,"baseStats.spirit.effective","gear_spirit=" );
    optionStr+= chkValue( src,"baseStats.armor.effective","gear_armor=" );
    optionStr+= chkValue( src,"spell.manaRegen.casting","gear_mp5=" );

    optionStr+= chkMaxValue( src,"spell.bonusHealing.value","gear_spell_power=" ); //maybe max(all spells)
    optionStr+= chkMaxValue( src,"melee.power.effective,ranged.power.effective","gear_attack_power=" );
    optionStr+= chkMaxValue( src,"melee.expertise.rating","gear_expertise_rating=" );
    optionStr+= chkMaxValue( src,"spell.hitRating.penetration,melee.hitRating.penetration,ranged.hitRating.penetration","gear_armor_penetration_rating=" );
    optionStr+= chkMaxValue( src,"spell.hasteRating.hasteRating,melee.mainHandSpeed.hasteRating,ranged.speed.hasteRating","gear_haste_rating=" );
    optionStr+= chkMaxValue( src,"spell.hitRating.value,melee.hitRating.value,ranged.hitRating.value","gear_hit_rating=" );
    optionStr+= chkMaxValue( src,"spell.critChance.rating,melee.critChance.rating,ranged.critChance.rating","gear_crit_rating=" );
  }

  std::string glyphs="";

  // retrieve armory talents data (XML) for this player
  if ( !gearOnly )
  {
    std::string src2= getArmoryData( aURL, UPG_TALENTS );
    src2= getNode( src2, "talentGroups" );
    node= getNode( src2,"talentGroup" );
    std::string active= getValue( node,"active" );
    if ( ((active!="1")&&!inactiveTalents)|| ((active=="1")&&inactiveTalents) )
    {
      node= getNodeOne( src2,"talentGroup",2 );
    }
    optionStr+= chkValue( node, "talentSpec.value", "talents=http://worldofwarcraft?encoded=" );
    glyphs = getNode( node, "glyphs" );
  }else{
    glyphs = getNode( src, "characterTab.glyphs" );
  }

  // submit options so far, in order to create player, because following parses may need to call player->parse
  if ( optionStr!="" )
  {
    if ( debug ) printf( "%s", optionStr.c_str() ); ;
    char* buffer= new char[optionStr.length()+20];
    strcpy( buffer,optionStr.c_str() );
    option_t::parse_line( sim, buffer );
    delete buffer;
    aURL.generated_options+=optionStr;
    optionStr="";
  };

  // check if we have active player. It either should be set from before, or after last parse of "player="
  if ( !sim->active_player ){
    printf("No active player in Armory parse ! Last player name: %s\n",charName.c_str());
    return true;
  }

  // parse for glyphs
  if ( ( glyphs!="" )&&( sim->active_player ) )
  {
    std::string glyph_node, glyph_name;
    for ( unsigned int i=1; i<=3; i++ )
    {
      glyph_node= getNodeOne( glyphs, "glyph",i );
      if ( glyph_node!="" )
      {
        glyph_name= getValue( glyph_node, "name" );
        if ( glyph_name !="" )
        {
          // call directly to set option. It should NOT return error if not found
          player_parse_option( aURL, proper_option_name( glyph_name ),"1" );
        }
      }
    }
  }

  // parse items for effects, procs etc
  node = getNode( src, "characterTab.items" );
  if ( ( node!="" )&&( sim->active_player ) )
  {
    gear_stats_t gs, oldGS;
    unsigned int nGs=0;
    std::string item_node, item_id;
    for ( unsigned int i=1; i<=18; i++ )
    {
      item_node= getNodeOne( node, "item",i );
      addItemGlyphOption( aURL, item_node, "id" );
      addItemGlyphOption( aURL, item_node, "gem0Id" );
      addItemGlyphOption( aURL, item_node, "gem1Id" );
      addItemGlyphOption( aURL, item_node, "gem2Id" );
      addItemGlyphOption( aURL, item_node, "permanentenchant" );
      // if we parse each item for stats , do it here
      if ( ParseEachItem&&( item_node!="" ) )
      {
        std::string  item_id= getValue( item_node, "id" );
        std::string  item_slot= getValue( item_node, "slot" );
        if ( parseItemStats( aURL, gs, item_id, item_slot ) ) nGs++;
        if ( debug>1 )
        { // detailed, show every item stat gains
          printf( "ITEM= %s & slot=%s\n", item_id.c_str(),item_slot.c_str() );
          displayStats( gs, &oldGS );
          oldGS=gs;
        }
      }
    }
    //copy gear stats if needed
    if ( ( nGs>0 )&& ParseEachItem ){
      copy_gear_to_player(aURL, gs);
    }
  }else{
    printf("No <characterTab.items> found in Armory for player: %s\n",charName.c_str());
  }

  if ( debug && sim->active_player) displayStats( sim->active_player->gear_stats );
  // now parse remaining options, if any
  if ( optionStr!="" )
  {
    if ( debug ) printf( "%s", optionStr.c_str() ); ;
    char* buffer= new char[optionStr.length()+20];
    strcpy( buffer,optionStr.c_str() );
    option_t::parse_line( sim, buffer );
    delete buffer;
    aURL.generated_options+=optionStr;
  }
  //save player simcraft if requested
  if (save_player) save_player_simcraft(aURL);
  //return
  if ( debug )
  {
    char c;
    printf( "<debug> Press any key ...\n" );
    scanf( "%c",&c );
  }
  return true;
}

// parse multiple players in same line, or alternate armory profile (!)
bool parseArmoryPlayers( sim_t* sim, std::string URL ){
  std::vector<std::string> splits;
  unsigned int num = util_t::string_split( splits, URL, "," );
  if (num<2) return false;
  std::string srvURL=splits[0];
  unsigned int i=1;
  if (srvURL.find("?r=")==string::npos){
      srvURL+="/?r="+splits[1];
      i++;
  }
  // parse players
  while ( i < num )
  {
    std::string player=splits[i];
    //check if save request
    bool toSave=false;
    if (tolower(splits[i])=="save") {
      i++;
      toSave=true;
      if (i>=num) break;
      player=splits[i];
    }
    bool inactiveTalents=false;
    if (player!=""){
        //check if !Name is passed to require alternate talents
        if (player[0]=='!'){
            inactiveTalents=true;
            player=player.substr(1);
        }
        //call parse of one player
        parseArmory( sim, srvURL+"&n="+player, inactiveTalents, false, toSave );
    }
    i++;
  }
  return true;
}


bool  replace_item(sim_t* sim, std::string& new_id_str,std::string& opt_slot,std::string& opt_gems,std::string& opt_ench){
  printf("Replacing ID=%s , slot=%s , gems=\"%s\" , ench=\"%s\" \n",new_id_str.c_str(), opt_slot.c_str(), opt_gems.c_str(), opt_ench.c_str());
  return true;
}


// replace previously parsed item(s) with new one
bool parseItemReplacement(sim_t* sim,  const std::string& item_list){
  std::vector<std::string> splits;
  int num = util_t::string_split( splits, item_list, "/\\" );
  for (int i=0; i < num ; i++)
  if (splits[i]!="")
  {
    //split one item to options
    std::vector<std::string> options;
    int num_opt = util_t::string_split( options, splits[i], "," );
    if (num_opt<=0) continue;
    // get new id
    std::string new_id_str=options[0];
    int new_id=atoi(new_id_str.c_str());
    if (new_id<=0){
      // try to find new id in URL, after ?i= or &i=
      
    }
    // check options
    std::string opt_slot="";
    std::string opt_gems="";
    std::string opt_ench="";
    for (int u=1; u<num_opt; u++){
      std::string opt= options[u];
      std::string::size_type idx= opt.find("=");
      if (idx!=string::npos){
        std::string value= opt.substr(idx+1);
        opt.erase(idx);
        opt= tolower(opt);
        if (opt=="slot") opt_slot=value; else
          if ((opt=="gem")||(opt=="gems"))  opt_gems+=(opt_gems!=""?" and ":"")+value; else
        if ((opt=="ench")||(opt=="enchant"))  opt_ench+=(opt_ench!=""?" and ":"")+value; else
        {
          printf("Unknown option: %s in %s\n", options[u].c_str(), splits[i].c_str());
          assert(0);
        }
      }
    }
    //replace underscores with spaces (since spaces are not allowed in simcraft options)
    replace_char( opt_slot ,'_',' ');
    replace_char( opt_gems ,'_',' ');
    replace_char( opt_ench ,'_',' ');
    // now parse and replace new item
    replace_item(sim, new_id_str, opt_slot, opt_gems, opt_ench);
  }
  return true;
}

bool armory_option_parse(sim_t* sim,  const std::string& name, const std::string& value){
  if ( name=="player" )             return parseArmory( sim, value);
  if ( name=="gear" )               return parseArmory( sim, value, false, true );
  if ( name=="armory" )             return parseArmoryPlayers( sim, value);
  if ( name=="new_item_id" )        return parseItemReplacement( sim, value);
  if ( name=="http_cache_clear") {
    http_t::cache_clear();
    return true;
  }
  return false;
}



struct set_tiers_t
{
  std::string setName;
  unsigned int tier;
};

// get set tier based on name
// probably only tiers 7&8 are important here atm
// set names should be same as on Armory - case sensitive, and common for 10man/25man
unsigned int  getSetTier( std::string setName )
{
  set_tiers_t setTiers[]=
  {
    // death knight sets
    {"scourgeborne",7},
    {"darkruned",8},
    // druid sets
    {"malorne",4},
    {"nordrassil",5},
    {"thunderheart",6},
    {"dreamwalker",7},
    {"nightsong",8},
    // hunter sets
    {"cryptstalker",7},
    {"scourgestalker",8},
    // mage sets
    {"frostfire",7},
    {"kirin'dor",8},
    // priest sets
    {"faith",7},
    {"sanctification",8},
    // rogue sets
    {"bonescythe",7},
    {"terrorblade",8},
    // shaman sets
    {"earthshatter",7},
    {"worldbreaker",8},
    // warlock sets
    {"felheart",1},
    {"nemesis",2},
    {"plagueheart raiment",3},
    {"voidheart",4},
    {"corruptor",5},
    {"malefic",6},
    {"plagueheart",7},
    {"deathbringer",8},
    // warrior sets
    {"dreadnaught",7},
    {"siegebreaker",8},
    //end of list
    {"",0}
  };
  // find set tier based on name
  unsigned int tier=0;
  setName=" "+tolower(setName)+" ";
  for ( size_t i=0; setTiers[i].tier; i++ ){
    std::string tierName=" "+setTiers[i].setName+" ";
  	if ( setName.find(tierName)!=string::npos )
    {
      tier=setTiers[i].tier;
      break;
    }
  }
  return tier;
}


