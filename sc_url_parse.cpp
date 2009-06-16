// internal debug flag, only for this unit
// 0=off
// 1=regular debug
// 2=more detailed debug, each item stat shown
// 3=each URL read shown
static int debug=0;


#include "simcraft.h"
using namespace std;

const bool ParseEachItem=true;

struct armory_item_t{
  std::string id;
  std::string name;
  int slot;
  std::string inv_type;
  gear_stats_t gear;
  bool has_enchants;
  gear_stats_t enchants;
  int n_gems;
  gear_stats_t gems[6];
  bool has_bonus;
  gear_stats_t gem_bonus;
  std::string clicky_str;
  armory_item_t() : slot(0), has_enchants(false), n_gems(0), has_bonus(false) {}
};

struct urlSplit_t
{
  std::string wwwAdr;
  std::string realm;
  std::string player;
  int srvType;
  sim_t* sim;
  std::string generated_options;
  unsigned int setPieces[20];
  player_t* active_player;
  armory_item_t item_stats[20];
  bool saved;
  int n_clickies;
  urlSplit_t() : srvType(0), sim(0), active_player(0), saved(false), n_clickies(0)
  {
    memset( setPieces, 0x00, sizeof(setPieces) );
  }
};

enum url_page_t {  UPG_GEAR, UPG_TALENTS, UPG_ITEM  };


// utility functions
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
std::string getArmoryData( urlSplit_t& aURL, url_page_t pgt, std::string moreParams="" )
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
// remove apostrophes and colons
std::string proper_option_name( const std::string& full_name )
{
  if ( full_name=="" ) return full_name;
  // first to lower letters and _ for spaces
  std::string newName( full_name.length(),' ' );
  newName="";
  for ( size_t i=0; i<full_name.length(); i++ )
  {
    char c=full_name[i];
    c= tolower( c );            // lower case
    if ( c==' ' ) c='_';        // spaces to underscores
    if ( c!='\'' && c!=':' )      // remove apostrophes and colons
      newName+=c;
  }
  // then remove first "of" for glyphs (legacy)
  if ( newName.substr( 0,9 )=="glyph_of_" )   newName.erase( 6,3 );
  // return result
  return newName;
}

// convert full descriptive name into "option name"
// spaces become underscores, all lower letters
// remove apostrophes
std::string talent_calc_class_name( const std::string& full_name )
{
  if ( full_name=="" ) return full_name;
  // first to lower letters and _ for spaces
  std::string newName( full_name.length(),' ' );
  newName="";
  for ( size_t i=0; i<full_name.length(); i++ )
  {
    char c=full_name[i];
    c= tolower( c );            // lower case
    if ( c!='\'' && c!=':' && c!=' ' )      // remove apostrophes
      newName+=c;
  }
  // return result
  return newName;
}



//split URL to server, realm, player
bool splitURL( const std::string& URL, urlSplit_t& aURL )
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
  aURL.srvType=1;
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
std::string getNodeOne( const std::string& src, const std::string& name, int occurence=1 )
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
std::string getNode( const std::string& src, const std::string& nodeList )
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
std::string getValueOne( const std::string& node, const std::string& name )
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
std::string getValue( const std::string& src, std::string path )
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
std::string getMaxValue( const std::string& src, const std::string& path )
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
double getParamFloat( const std::string& src, const std::string& path )
{
  std::string res= getValue( src, path );
  return atof( res.c_str() );
}

//get float value from entire node
double getNodeFloat( const std::string& src, const std::string& path )
{
  std::string res= getNode( src, path );
  size_t idB= res.find( ">" ); // for <armor armorBonus="0">155</armor>
  if ( ( idB != string::npos )&&( idB<( res.length()-1 ) ) )
    res.erase( 0,idB+1 );
  return atof( res.c_str() );
}


std::string chkValue( const std::string& src, const std::string& path, const std::string& option )
{
  std::string res=getValue( src, path );
  if ( res!="" )
    return option+res+"\n";
  else
    return "";
}

std::string chkMaxValue( const std::string& src, const std::string& path, const std::string& option )
{
  std::string res=getMaxValue( src, path );
  if ( res!="" )
    return option+res+"\n";
  else
    return "";
}


unsigned int  getSetTier( std::string setName );




// call players parse option
std::string player_parse_option( urlSplit_t& aURL, const std::string& name, const std::string& value )
{
  std::string opt_str="";
  if ( !aURL.sim->active_player ) return opt_str;
  bool ok=aURL.sim->active_player->parse_option( name,value );
  if (ok){
    opt_str= name +"="+ value +"\n";
    aURL.generated_options+=opt_str;
    if ( debug ) printf("%s",opt_str.c_str() );
  }
  return opt_str;
}

// call player_parse_option for every item, if special option exists
// useful only for GEMs and ENCHANTs
// since for items and glyphs "proper names" are generated and tried as options
void addItemGlyphOption( urlSplit_t& aURL, const std::string&  node, const std::string& name, std::string opt_value="1" )
{
  if ( node!="" )
  {
    std::string itemID= getValue( node, name );
    if ( itemID !="" )
    {
      //rename few well known gems, to be more readable
      if (itemID=="41285") itemID="chaotic_skyflare"; else
      if (itemID=="41333") itemID="ember_skyflare"; else
      if (itemID=="41398") itemID="relentless_earthsiege"; else
      if (itemID=="41380") itemID="austere_earthsiege"; else
      itemID="item_"+itemID;
      //call parse to check if this item option exists
      player_parse_option( aURL,itemID,opt_value );
    }
  }
}




// display all stats
void displayStats_one( const std::string& comment, gear_stats_t& gs )
{
  char buff[200];
  sprintf(buff,"   %10s : ",comment.c_str());
  std::string s=buff;
  int old_len=0;
  for ( unsigned int i=STAT_NONE; i<STAT_MAX; i++ )
  {
    double val=gs.get_stat( i );
    if ( val!=0 ){
      sprintf( buff, "%s=%1.0lf, ",util_t::stat_type_string( i ), val );
      if (strlen(buff)+s.length()-old_len>78){
        old_len=s.length();
        s+="\n                ";
      }
      s+=buff;
    }
  }
  printf("%s\n",s.c_str());
}

void displayStats( armory_item_t& gs )
{
  printf( "ITEM=%6s/%2d: %s\n", gs.id.c_str(), gs.slot, gs.name.c_str() );
  displayStats_one("gear", gs.gear);
  if (gs.has_enchants)
    displayStats_one("enchants", gs.enchants);
  if (gs.n_gems>0){
    gear_stats_t sum_gems;
    char buff[200];
    for (int i=1; i<=gs.n_gems; i++) sum_gems+=gs.gems[i];
    sum_gems+=gs.gem_bonus;
    sprintf(buff,"gems(%d)",gs.n_gems);
    displayStats_one(buff, sum_gems);
  }
  printf("\n");
}



// adds option for set bonuses
void  addSetInfo( urlSplit_t& aURL, const std::string& setName, unsigned int setPieces, const std::string& item_id )
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
double oneTxtStat( std::string txt, const std::string& fullpat, int dir, int type, double* for_value=0  )
{
  size_t idx=0;
  double value=0;
  size_t idL, idR;
  //support multiple occurences of same pattern
  while ( idx != string::npos )
  {
    idx= txt.find( fullpat );
    if ( idx != string::npos )
    {
      idL=idx;
      idR=idx+fullpat.length();
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
  // if this was "Use:" type, check if there is duration, " for 20 sec." 
  if (for_value){
    *for_value=0;
    if ((value!=0)&&(dir>0)){
      std::string for_str;
      int found_sz=-1;
      //try to find any variation of "for XX seconds"
      for_str=" for the next ";
      if ((found_sz<0)&&(txt.substr(0,for_str.length())==for_str)) found_sz=for_str.length();
      for_str=" for next ";
      if ((found_sz<0)&&(txt.substr(0,for_str.length())==for_str)) found_sz=for_str.length();
      for_str=" for ";
      if ((found_sz<0)&&(txt.substr(0,for_str.length())==for_str)) found_sz=for_str.length();
      // if found, extract value
      if (found_sz>0){
        txt.erase(0,found_sz);
        size_t dL=0;
        size_t dR=0;
        size_t p=0;
        while ( ( p<txt.length() )&&( txt[p]==' ' ) ) p++; //skip spaces
        dL=dR=p;
        while ( ( p<txt.length() )&& my_isdigit( txt[p] ) ) p++; //walk over number
        dR=p;
        //place result
        *for_value= atof (txt.substr( dL,dR-dL ).c_str());
      }
    }
  }
  return value;
}

// Try to find txt pattern and value pair.  This looks for "+X to All Stats" and adds all the stats
// If found, it will remove the pattern/value from txt
const bool chkAllStatsBonus( gear_stats_t& gs, const std::string& txt, int type , const std::string& pattern)
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
bool chkOneTxtStat( gear_stats_t& gs, const std::string& txt, int type , double* for_value, int statID, const std::string& pattern )
{
  bool ok=false;

  std::string pat;
  double val=0;
  // this patterns should increase stat
  std::string pref_patterns[]=
  {
    " improves your ",
    " improves ",
    " increases your ",
    " increases ",
    ""
  };
  int i=0;
  while ((!ok)&&(pref_patterns[i]!="")){
    pat= pref_patterns[i]+pattern+" by ";
    double for_val_tmp=0;
    val= oneTxtStat( txt, pat, +1, type, &for_val_tmp );
    if ( val )
    {
      gs.add_stat( statID,  val );
      if (for_value) *for_value=for_val_tmp;
      ok=true;
    }
    i++;
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
void addTextStats( gear_stats_t& gs, std::string txt, int type, double* for_value=0 )
{
  if ( txt=="" ) return;
  txt= " "+tolower( txt )+" ";

  //chkOneTxtStat(gs,txt, STAT_SPELL_POWER,               "shadow spell damage"); //problem: item may have shdw+frost+fire...
  chkAllStatsBonus( gs,txt,type,                              "all stats" );
  chkOneTxtStat( gs,txt,type,for_value, STAT_SPELL_POWER,               "spell power" );
  chkOneTxtStat( gs,txt,type,for_value, STAT_MP5,                       "mana regen" );
  chkOneTxtStat( gs,txt,type,for_value, STAT_MP5,                       "mana every" );
  chkOneTxtStat( gs,txt,type,for_value, STAT_MP5,                       "mana per" );
  chkOneTxtStat( gs,txt,type,for_value, STAT_MP5,                       "mana restored per" );
  chkOneTxtStat( gs,txt,type,for_value, STAT_MP5,                       "mana/5" );
  chkOneTxtStat( gs,txt,type,for_value, STAT_ATTACK_POWER,              "attack power" );
  chkOneTxtStat( gs,txt,type,for_value, STAT_EXPERTISE_RATING,          "expertise rating" );
  chkOneTxtStat( gs,txt,type,for_value, STAT_ARMOR_PENETRATION_RATING,  "armor penetration" );
  chkOneTxtStat( gs,txt,type,for_value, STAT_HASTE_RATING,              "haste rating" );
  chkOneTxtStat( gs,txt,type,for_value, STAT_HIT_RATING,                "ranged hit rating" ); //we dont have them separate
  chkOneTxtStat( gs,txt,type,for_value, STAT_HIT_RATING,                "hit rating" );
  chkOneTxtStat( gs,txt,type,for_value, STAT_CRIT_RATING,               "ranged critical strike" );
  chkOneTxtStat( gs,txt,type,for_value, STAT_CRIT_RATING,               "critical strike rating" );
  chkOneTxtStat( gs,txt,type,for_value, STAT_CRIT_RATING,               "crit rating" );
  chkOneTxtStat( gs,txt,type,for_value, STAT_STRENGTH,                  "strength" );
  chkOneTxtStat( gs,txt,type,for_value, STAT_AGILITY,                   "agility" );
  chkOneTxtStat( gs,txt,type,for_value, STAT_STAMINA,                   "stamina" );
  chkOneTxtStat( gs,txt,type,for_value, STAT_INTELLECT,                 "intellect" );
  chkOneTxtStat( gs,txt,type,for_value, STAT_SPIRIT,                    "spirit" );
  //shorter, less safe parses
  chkOneTxtStat( gs,txt,type,for_value, STAT_ARMOR,                     "armor" );
  chkOneTxtStat( gs,txt,type,for_value, STAT_HEALTH,                    "health" );
  chkOneTxtStat( gs,txt,type,for_value, STAT_MANA,                      "mana" );

}


// parse stats from one item and adds it to total stats in "gs"
// also should parse weapon stats
bool  parseItemStats( urlSplit_t& aURL, armory_item_t& gs,  const std::string& item_id, const std::string& item_slot )
{
  if ( item_id=="" ) return false;
  std::string ref= "&i="+item_id;
  if (item_slot!="") ref+="&s="+item_slot;
  std::string src= getArmoryData( aURL,UPG_ITEM, ref );
  if ( src=="" ){
    printf("\nFailed to read web page: %s\n",aURL.wwwAdr.c_str());
    return false;
  }
  gs.id=item_id;
  gs.name= getNode(src, "name"); 
  gs.inv_type= getNode(src, "equipData.inventoryType"); 
  gs.slot= atoi(item_slot.c_str());
  // add fixed stats
  gs.gear.add_stat( STAT_STRENGTH,                  getNodeFloat( src, "bonusStrength" ) );
  gs.gear.add_stat( STAT_AGILITY,                   getNodeFloat( src, "bonusAgility" ) );
  gs.gear.add_stat( STAT_STAMINA,                   getNodeFloat( src, "bonusStamina" ) );
  gs.gear.add_stat( STAT_INTELLECT,                 getNodeFloat( src, "bonusIntellect" ) );
  gs.gear.add_stat( STAT_SPIRIT,                    getNodeFloat( src, "bonusSpirit" ) );
  gs.gear.add_stat( STAT_SPELL_POWER,               getNodeFloat( src, "bonusSpellPower" ) );
  gs.gear.add_stat( STAT_MP5,                       getNodeFloat( src, "bonusManaRegen" ) );
  gs.gear.add_stat( STAT_ATTACK_POWER,              getNodeFloat( src, "bonusAttackPower" ) );
  gs.gear.add_stat( STAT_EXPERTISE_RATING,          getNodeFloat( src, "bonusExpertiseRating" ) );
  gs.gear.add_stat( STAT_ARMOR_PENETRATION_RATING,  getNodeFloat( src, "bonusArmorPenetration" ) );
  gs.gear.add_stat( STAT_HASTE_RATING,              getNodeFloat( src, "bonusHasteRating" ) );
  gs.gear.add_stat( STAT_HIT_RATING,                getNodeFloat( src, "bonusHitRating" ) );
  gs.gear.add_stat( STAT_CRIT_RATING,               getNodeFloat( src, "bonusCritRating" ) );
  gs.gear.add_stat( STAT_ARMOR,                     getNodeFloat( src, "armor" ) );
  // add textual - descriptive - stats
  std::string node= getNode( src, "enchant" );
  addTextStats( gs.enchants, node,1 );
  player_parse_option( aURL, proper_option_name(node), "1" ); // for named enchants like "Lightweave Embroidery"
  gs.has_enchants= (node!="");
  // spell data- can have multiple spells, both "Equip" or "Use"
  node= getNode( src, "spellData" );
  for ( unsigned int i=1; i<=5; i++ )
  {
    std::string spell= getNodeOne( node, "spell" ,i );
    if ( spell!="" )
    {
      if (getNode(spell,"trigger")=="1"){
        // Equip: parse stats, but avoid "have chance to" - by including only those starting with "increases.."
        addTextStats( gs.gear, getNode( spell, "desc" ),2 );
      }else{
        //Use: try to recognize if it is increase of some stat for XX seconds on use, to put in Clicky_X
        gear_stats_t tmp_gear;
        double for_value=0;
        addTextStats( tmp_gear, getNode( spell, "desc" ),5, &for_value );
        if (for_value>0){
          // if found, check for recognizable stats to form "trigger" action
          std::string clicky_str="";
          std::string clicky_val="";
          if (aURL.active_player->clicky_1=="") clicky_str="clicky_1"; else
          if (aURL.active_player->clicky_2=="") clicky_str="clicky_2"; else
          if (aURL.active_player->clicky_3=="") clicky_str="clicky_3"; 
          if (clicky_str!=""){
            char buff[200];
            sprintf(buff,",length=%d,cooldown=120",(int)for_value);
            std::string clicky=buff;
            if (tmp_gear.spell_power>0){
              sprintf(buff,"spell_power_trinket,power=%d", (int) tmp_gear.spell_power);
              clicky_val=buff+clicky;
            }else
            if (tmp_gear.attack_power>0){
              sprintf(buff,"attack_power_trinket,power=%d", (int) tmp_gear.attack_power);
              clicky_val=buff+clicky;
            }else
            if (tmp_gear.haste_rating>0){
              sprintf(buff,"haste_trinket,rating=%d", (int) tmp_gear.haste_rating);
              clicky_val=buff+clicky;
            };
            //set option
            if (clicky_val!=""){
              player_parse_option( aURL,clicky_str, clicky_val );
              gs.clicky_str=clicky_str;
            }
          }else{
            printf("Too many clickies for player %s\n",aURL.active_player->name());
          }
        }

      }
    }else
      break;
  }
  // parse gems and sockets
  node= getNode( src, "socketData" );
  bool allMatch=true;
  for ( unsigned int i=1; i<=5; i++ )
  {
    std::string gem= getNodeOne( node, "socket" ,i );
    if ( gem!="" )
    {
      gs.n_gems++;
      addTextStats( gs.gems[gs.n_gems], getValue( gem, "enchant" ),3 );
      if ( getValue( gem, "match" )!="1" ) allMatch=false;
    }
  }
  // add socket bonus if all match
  if ( allMatch )
  {
    addTextStats( gs.gem_bonus, getNode( node, "socketMatchEnchant" ),4 );
    gs.has_bonus=true;
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
    std::string wpnValue= "";
    std::string wpnClass=tolower( getNode( src, "equipData.subclassName" ) );
    // try to match wpn class name to our names
    size_t idx=wpnClass.find(" ");
    if ( idx != string::npos) wpnClass.erase(idx);
    bool is_2h= getNode( src, "equipData.inventoryType" ) == "17";
    if (is_2h){ // try first to find _2H version if Two Hander
      std::string wpnClass_2h=wpnClass+"_2h";
      for (int i=0; i<WEAPON_MAX && wpnValue==""; i++)
        if (util_t::weapon_type_string(i)==wpnClass_2h)
          wpnValue=wpnClass_2h;
    }
    if (wpnValue=="") // if not 2H, or not named (staff_2h), try without
      for (int i=0; i<WEAPON_MAX && wpnValue==""; i++)
        if (util_t::weapon_type_string(i)==wpnClass)
          wpnValue=wpnClass;
    //add rest of weapon data
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

gear_stats_t sum_item_stats(armory_item_t& aitem){
  gear_stats_t gs=aitem.gear;
  gs+=aitem.enchants;
  for (int i=1; i<=aitem.n_gems; i++) gs+=aitem.gems[i];
  gs+=aitem.gem_bonus;
  return gs;
}

// copy gear stats toplayer and form option str
bool  copy_gear_to_player(urlSplit_t& aURL){
  if (!aURL.sim->active_player) return false;
  gear_stats_t gs;
  for (int i=0; i<20; i++) gs+= sum_item_stats(aURL.item_stats[i]);
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
  aURL.saved=true;
  thread_t::unlock();
  return true;
}




// main procedure that parse armory info from giuven player URL (URL must have realm/player inside)
// it set options either by building option string and calling parse_line(), or by directly calling
// player->parse_option()
bool parseArmory( sim_t* sim, const std::string& URL, bool inactiveTalents=false, bool gearOnly=false, bool save_player=false )
{
  if (!sim) return false;
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
  std::string className= talent_calc_class_name( node );
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
    std::string talentCalcString = "talents=http://talent.mmo-champion.com/?" + className + "=";
    optionStr+= chkValue( node, "talentSpec.value", talentCalcString );
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
  aURL.active_player= sim->active_player;

  // parse for glyphs
  if ( ( glyphs!="" )&&( sim->active_player ) )
  {
    std::string glyph_node, glyph_name;
    for ( unsigned int i=1; i<=6; i++ )
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
        int slot=atoi(item_slot.c_str());
        if ((slot>=0)&&(slot<20)){
          armory_item_t item_stats;
          if ( parseItemStats( aURL, item_stats, item_id, item_slot ) ){
            nGs++;
            aURL.item_stats[slot]=item_stats;
          }
          if ( debug>1 )
          { // detailed, show every item stat gains
            displayStats( item_stats);
          }
        }
      }
    }
    //copy gear stats if needed
    if ( ( nGs>0 )&& ParseEachItem ){
      copy_gear_to_player(aURL);
    }
  }else{
    printf("No <characterTab.items> found in Armory for player: %s\n",charName.c_str());
  }

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
  if (!sim->last_armory_player)
    sim->last_armory_player= new urlSplit_t;
  *sim->last_armory_player= aURL;
  //return
  if ( debug )
  {
    char c;
    printf( "<debug Armory Parse> Press any key ...\n" );
    scanf( "%c",&c );
  }
  return true;
}

// parse multiple players in same line, or alternate armory profile (!)
bool parseArmoryPlayers( sim_t* sim, const std::string& URL ){
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


bool  replace_item(sim_t* sim, const std::string& new_id_str, const std::string& opt_old_id, std::string opt_slot, const std::string& opt_gems, const std::string& opt_ench){
  if ((new_id_str=="")||(!sim)) return false;
  urlSplit_t* aURL=sim->last_armory_player;
  if (!aURL) return false;
  // define slot
  char buff[200];
  int slot=-1;
  if (opt_slot==""){
    // if old item id is given, search for it in list
    if (opt_old_id!=""){
      for (int i=0; i<20; i++)
        if (aURL->item_stats[i].id==opt_old_id){
          slot=i;
          break;
        }
    }
    if (slot>=0){
      sprintf(buff,"%d",slot);
      opt_slot=buff;
    }
  }else{
    slot=atoi(opt_slot.c_str());
    if (slot>=20){
      printf("Slot number too big: %s in %s\n",opt_slot.c_str(), new_id_str.c_str());
      return false;
    }
  }
  // replace old item, and try to turn off old options, if slot is known here
  armory_item_t item_stats;
  aURL->generated_options+="# New_item="+new_id_str+", slot="+opt_slot.c_str()+", old_item="+opt_old_id.c_str()+"\n";
  armory_item_t* item_place=0;
  armory_item_t old_stats;
  if (slot>=0){
    item_place= &aURL->item_stats[slot];
    old_stats= *item_place;
    if (item_place->name!="")   player_parse_option( *aURL, proper_option_name( item_place->name ),"0" );
    if (item_place->clicky_str!="") player_parse_option( *aURL, item_place->clicky_str,"" );
  }
  if (new_id_str!="0"){
    // this will parse new item, and also insert all item related options (name,clicky,weapons...)
    if ( parseItemStats( *aURL, item_stats, new_id_str, opt_slot ) ){
      // if slot was not found before, try to find based on inventory type
      if (slot<0){
        if (item_stats.inv_type!=""){
          for (int i=0; i<20; i++)
            if (aURL->item_stats[i].inv_type==item_stats.inv_type){
              slot=i;
              break;
            }
        }
        if (slot<0){
          printf("Could not find slot where to replace new item: (%s) %s\n",new_id_str.c_str(), item_stats.name.c_str());
          return false;
        }
      }
      // replace old item, and try to turn off old options, if slot was not known before
      if (!item_place){
        item_place= &aURL->item_stats[slot];
        old_stats= *item_place;
        if (item_place->name!="")   player_parse_option( *aURL, proper_option_name( item_place->name ),"0" );
        if (item_place->clicky_str!="") player_parse_option( *aURL, item_place->clicky_str,"" );
      }
      //set new values
      item_place->id=item_stats.id;
      item_place->name=item_stats.name;
      item_place->inv_type=item_stats.inv_type;
      item_place->gear=item_stats.gear;
      item_place->slot=slot;
      gear_stats_t gs;
      // if we have specified enchants
      if (opt_ench!=""){
        addTextStats( gs, opt_ench,1 );
        item_place->enchants=gs;
        item_place->has_enchants=true;
      }
      // if we have specified gems
      if (opt_gems!=""){
        addTextStats( gs, opt_gems,3 );
        item_place->gems[1]=gs;
        item_place->n_gems=1;
        item_place->has_bonus=false;
      }else{
        // if we keep old gems, limit to same number of gems
        //or fill up with +19SP
        if (item_stats.n_gems>item_place->n_gems)
          for (int i=item_place->n_gems+1; i<=item_stats.n_gems; i++){
            memset(&gs,0,sizeof(gs));
            addTextStats( gs, "+19 spell power",3 );
            item_place->gems[i]=gs;
          }
        item_place->n_gems=item_stats.n_gems;
      }

      // options comment
      sprintf(buff,"# Replaced ITEM=%s/%d : %s \n",old_stats.id.c_str(), old_stats.slot, old_stats.name.c_str());
      if (debug){
        printf(buff);
        displayStats( old_stats);
      }
      aURL->generated_options+=buff;
      sprintf(buff,"# with new ITEM=%s/%d : %s \n",item_place->id.c_str(), item_place->slot, item_place->name.c_str());
      if (debug){
        printf(buff);
        displayStats( *item_place);
      }
      aURL->generated_options+=buff;

    }
  }else{
    // if no new id was given, just remove old item at slot
    if (slot>=0){
      aURL->item_stats[slot]=item_stats;
    }
  }
  // now recalculate total stats
  copy_gear_to_player(*aURL);

  //save if it was previously saved
  if (aURL->saved) save_player_simcraft(*aURL);
  return true;
}


// replace previously parsed item(s) with new one
bool parseItemReplacement(sim_t* sim,  const std::string& item_list){
  std::vector<std::string> splits;
  int num = util_t::string_split( splits, item_list, "\\" );
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
    if ((new_id<=0)&&(new_id_str!="0")){
      // try to find new id in URL, after ?i= or &i=
      std::string new_id2=tolower(new_id_str);
      size_t idx=new_id2.find("?i=");
      if (idx==string::npos)
        idx=new_id2.find("&i=");
      if (idx!=string::npos){
        idx+=3;
        new_id2.erase(0,idx);
        size_t idx2= new_id2.find("&",idx);
        if (idx2!=string::npos) new_id2.erase(idx2);
        new_id=atoi(new_id2.c_str());
      }
      // if still didnt find ID, error
      if (new_id<=0){
        printf("Wrong Item ID %s in %s\n", new_id_str.c_str(), item_list.c_str());
        return false;
      }else{
        char buff[200];
        sprintf(buff,"%d",new_id);
        new_id_str=buff;
      }
    }
    // check options
    std::string opt_slot="";
    std::string opt_oldid="";
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
        if ((opt=="oldid")||(opt=="old_id")||(opt=="old_item")) opt_oldid=value; else
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
    if ((new_id>0)||(new_id_str=="0"))
      replace_item(sim, new_id_str, opt_oldid, opt_slot, opt_gems, opt_ench);
  }
  //return
  if ( debug )
  {
    char c;
    printf( "<debug Replace Item> Press any key ...\n" );
    scanf( "%c",&c );
  }
  return true;
}

bool armory_option_parse(sim_t* sim,  const std::string& name, const std::string& value){
  if ( name=="player" )             return parseArmory( sim, value);
  if ( name=="gear" )               return parseArmory( sim, value, false, true );
  if ( name=="armory" )             return parseArmoryPlayers( sim, value);
  if ( name=="new_item" )           return parseItemReplacement( sim, value);
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


