#include "simcraft.h"

#if defined( _MSC_VER )
#include <windows.h>
#include <wininet.h>
#endif

#if defined(USE_CURL)
#include <curl/curl.h>
#endif

#include <algorithm>
using namespace std;

#ifdef _MSC_VER
#define USER_AGENT_FOR_XML L"Mozilla/5.0 (Windows; U; Windows NT 5.1; en-US; rv:1.8.1.1) Gecko/20061204 Firefox/2.0.0.1"
#else
#define USER_AGENT_FOR_XML "Mozilla/5.0 (Windows; U; Windows NT 5.1; en-US; rv:1.8.1.1) Gecko/20061204 Firefox/2.0.0.1"
#endif

#ifdef USE_CURL
// This is the writer call back function used by curl
static int writer(char *data, size_t size, size_t nmemb, std::string *buffer)
{
    int result = 0;

    // Is there anything in the buffer?
    if (buffer != NULL)
    {
        // Append the data to the buffer
        buffer->append(data, size * nmemb);
        // How much did we write?
        result = size * nmemb;
    }

    return result;
}

#endif


std::string getURLsource(std::string URL){
    std::string res="";
    #if defined( _MSC_VER )
        HINTERNET hINet, hFile;
        hINet = InternetOpen(USER_AGENT_FOR_XML, INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0 );
        if ( !hINet )  return res;
        std::wstring wURL( URL.length(), L' ');
        std::copy(URL.begin(), URL.end(), wURL.begin());
        hFile = InternetOpenUrl( hINet, wURL.c_str(), NULL, 0, INTERNET_FLAG_RELOAD, 0 );
        if ( hFile )
        {
            const int bufSz=20000;
            CHAR buffer[bufSz];
            DWORD dwRead=bufSz;
            //if (HttpQueryInfo(hFile, HTTP_QUERY_RAW_HEADERS_CRLF, buffer, &dwRead, NULL)) res+=buffer;
            while ( InternetReadFile( hFile, buffer, bufSz-2, &dwRead ) )
            {
                if ( dwRead == 0 )   break;
                buffer[dwRead] = 0;
                res+=buffer;
            }
            InternetCloseHandle( hFile );
        }
        InternetCloseHandle( hINet );
    #elif defined(USE_CURL)
    	CURL *curl;
    	curl = curl_easy_init();
    	if(curl)
  	{
  		curl_easy_setopt(curl, CURLOPT_URL, URL.c_str());
  		curl_easy_setopt(curl, CURLOPT_USERAGENT, USER_AGENT_FOR_XML);
  		curl_easy_setopt(curl, CURLOPT_HEADER, 0);
  		curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
  		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writer);
  		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &res);

  		curl_easy_perform(curl);

  		curl_easy_cleanup(curl);
  	}
    #endif
    return res;
}

struct cacheHeader_t{
    int n;
    int sz1;
    int sz2;
    double t;
};

struct urlCache_t{
    std::string url;
    double time;
    std::string data;
};

const int maxCache=100;
const int expirationSeconds=15*60;

int N_cache=0;
urlCache_t* urlCache=0;
double lastReqTime=0;

void SaveCache(){
    if ((!urlCache)||(N_cache==0)) return;
    FILE* file = fopen( "url_cache.txt" , "w" ); 
    if (file==NULL) return;
    //mark expired and count all
    int n=0;
    double nowTime= time(NULL);
    for (int i=1; i<=N_cache; i++){
        bool expired = nowTime - urlCache[i].time > expirationSeconds;
        if (expired) 
            urlCache[i].time=0;
        else
            n++;
    }
    //save to file
    cacheHeader_t hdr;
    hdr.n=n;
    if (fwrite(&hdr,sizeof(hdr),1,file)){
        for (int i=1; i<=N_cache; i++)
        if (urlCache[i].time>0)
        {
            hdr.n=i;
            hdr.sz1= urlCache[i].url.length();
            hdr.sz2= urlCache[i].data.length();
            hdr.t= urlCache[i].time;
            if (!fwrite(&hdr,sizeof(hdr),1,file)) break;
            fwrite(urlCache[i].url.c_str(), urlCache[i].url.length(),1,file);
            fwrite(urlCache[i].data.c_str(), urlCache[i].data.length(),1,file);
        }
    }
    fclose(file);
}

void LoadCache(){
    if (urlCache) return;
    urlCache=new urlCache_t[maxCache+2];
    N_cache    =0;
    FILE* file = fopen( "url_cache.txt", "r" ); 
    if (file==NULL) return;
    cacheHeader_t hdr;
    if (fread(&hdr,sizeof(hdr),1,file)){
        int nel= hdr.n;
        for (int i=1; (i<=nel)&&!feof(file); i++){
            if (fread(&hdr,sizeof(hdr),1,file)){
                N_cache++;
                urlCache[N_cache].time= hdr.t;
                char* buffer= new char[max(hdr.sz1, hdr.sz2)+10];
                fread(buffer, hdr.sz1,1,file); // url
                buffer[hdr.sz1]=0;
                urlCache[N_cache].url=buffer;
                fread(buffer, hdr.sz2,1,file); // data
                buffer[hdr.sz2]=0;
                urlCache[N_cache].data=buffer;
                delete buffer;
            }
        }
    }
    fclose(file);
}



//wrapper for getURLsource, support for throttling and cache
std::string getArmoryData(std::string URL){
    std::string data="";
    //check cache
    LoadCache();
    int found=0;
    for (int i=1; (i<=N_cache) && (!found); i++)
        if (urlCache[i].url==URL)  
            found=i;
    double nowTime= time( NULL );
    bool expired = (!found) || (nowTime-urlCache[found].time > expirationSeconds);
    if (expired){
        //check if last request was more than 2sec ago
        while ((lastReqTime<nowTime)&&(time( NULL )-lastReqTime<2));
        // HTTP request
        data= getURLsource(URL);
        //add to cache
        if (found){
            urlCache[found].time= nowTime;
            urlCache[found].data=data;
        }else
        if (N_cache<maxCache)
        {
            N_cache++;
            urlCache[N_cache].time=nowTime;
            urlCache[N_cache].url= URL;
            urlCache[N_cache].data= data;
        }
        return data;
    }else{
        //return from cache
        return  urlCache[found].data;
    }
}




// retrieve node from XML
// every node must start with <name , and then there are options:
// <name> xxxx </name> or  <name  a,b,c /> or  <name/>
std::string getNodeOne(std::string& src, std::string name, int occurence=1){
	std::string nstart="<"+name;
	//fint n-th occurence of node
	int offset=0;
	int idx=-1;
	do{
        std::string allowedNext=" >/";
	    idx=src.find(nstart,offset);
        bool found=false;
        while ((idx>=0)&&(!found))
        {
            char n=src[idx+nstart.length()];
            int i2=allowedNext.find(n);
            if (i2>=0)
                found=true;
            else
        		idx=src.find(nstart,idx+1);
        }
		occurence--;
		offset=idx+1;
	}while((idx>=0)&&(occurence>0));
	// if node start found, find end
	if ((idx>=0)&&(occurence==0)){
        int np=nstart.length();
		std::string nextChar=src.substr(idx+np,1);
		int idxEnd=-1;
        bool singleLine=true;
		if (nextChar==">")	
			singleLine=false;
        else{
            int idLn= src.find("\n",idx+1);
            int idEnd=src.find("/>",idx+1);
            if ((idEnd<0)||(idEnd>idLn)) singleLine=false;
        }
		if (singleLine)	
			nstart="/>"; // for single line nodes, it ends with />  ..and presume NO inline nodes
		else
			nstart="</"+name+">"; // if this is multiline node, it ends with </name>
		idxEnd=src.find(nstart,idx+1);
        int i1=idx+name.length()+2;
		if (idxEnd>i1){
			std::string res= src.substr(i1,idxEnd-i1);
			return res;
		}
	}
	return "";
}




// allow node path to get subnodes, from nodeList= "node1.node2.node3"
std::string getNode(std::string& src, std::string nodeList){
    std::string res=src;
    std::string node2;
    int last_find=0;
    int idx= nodeList.find(".",last_find);
    //get all nodes before last
    while (idx>last_find){
        node2= nodeList.substr(last_find,idx-last_find);
        res=getNodeOne(res, node2);
        last_find=idx+1;
        idx= nodeList.find(".",last_find);
    }
    //get last node
    node2= nodeList.substr(last_find,nodeList.length()-last_find);
    res=getNodeOne(res, node2);
    return res;
}


// retrieve string value from XML single line node.
// example: increasedHitPercent="12.47" penetration="0" reducedResist="0" value="327"
std::string getValueOne(std::string& node, std::string name){
    std::string res="";
    std::string src=" "+node;
	std::string nstart=" "+name+"=\"";
	int	idx=src.find(nstart);
	if (idx>=0){
        int i1=idx+nstart.length();
		int idxEnd=src.find("\"",i1);
		if (idxEnd>i1){
			res= src.substr(i1,idxEnd-i1);
		}
	}
	return res;
}

//return parameter value, given path to parameter
// example:  spell.bonusDamage.petBonus.damage
// where last is name of parameter
std::string getValue(std::string& src, std::string path){
    //find parameter name (last)
    int idx=path.rfind(".");
    if (idx>0){
        // get param name
        std::string paramName= path.substr(idx+1,path.length()-idx);
        //get subnode where param is
        path=path.substr(0,idx);
        std::string subnode= getNode(src, path);
        //return param
        return getValueOne(subnode, paramName);
    }else
        // if only parameter name is given, no dots, for example "damage"
        return getValueOne(src,path); 
}


// get max value out of list of paths, separated by ,
//example:  "spell.power,melee.power"
std::string getMaxValue(std::string& src, std::string path){
    std::string res="";
    double best=0;
    std::vector<std::string> paths;
    int num_paths = util_t::string_split( paths,path, "," );
    for( int i=0; i < num_paths; i++ ){
        std::string res2=getValue(src, paths[i]);
        if (res2!=""){
            double val=atof(res2.c_str());
            if ((res=="")||(val>best)){
                best=val;
                res=res2;
            }
        }
    }
    return res;
}


std::string chkValue(std::string& src, std::string path, std::string option){
    std::string res=getValue(src, path);
    if (res!="")
        return option+res+"\n";
    else
        return "";
}

std::string chkMaxValue(std::string& src, std::string path, std::string option){
    std::string res=getMaxValue(src, path);
    if (res!="")
        return option+res+"\n";
    else
        return "";
}


std::string checkItemGlyphOption(aef_type t, std::string id_name);


std::string parseItemGlyphOption(armor_effect_t* table, int* setCounters, aef_type t, std::string id_name){
    std::string my_options="";
    if (table!=NULL){
        int i=0;
        while (table[i].t!=AEF_NONE){
            if ((table[i].t==t)&&(table[i].id_name==id_name)){
                std::string theOption=table[i].option;
                if (theOption!="")  my_options+= theOption+"\n";
                //is this set item?
                if ((table[i].set>0)&&(table[i].set<20)){
                    setCounters[table[i].set]++;
					int ns=setCounters[table[i].set];
					if ((ns==2)||(ns==4)){
						char setName[100];
						sprintf(setName,"tier%d_%dpc=1\n",table[i].set , setCounters[table[i].set]);
						theOption=setName;
						my_options+=theOption;
					}
                }
            }
            i++;
        }
    }
    return my_options;
}


std::string addItemGlyphOption(sim_t* sim, std::string&  node, std::string name, aef_type t){
	std::string res="";
    if (node!=""){
		std::string value= getValue(node, name);
        if (value !=""){
            res+=checkItemGlyphOption(t, value);
            if ( sim->active_player) 
                res+=sim->active_player->checkItemGlyphOption(t, value);
        }
    }
	return res;
}



bool parseArmory(sim_t* sim, std::string URL, bool parseName, bool parseTalents, bool parseGear){
    std::string optionStr="";
    bool debug=false;
    // split URL
    int iofs=0;
    size_t id_http= URL.find("http://",iofs);
    if (id_http != string::npos) iofs=7;
    size_t id_folder= URL.find("/", iofs);
    if (id_folder != string::npos) iofs=id_folder+1;
    size_t id_srv= URL.find("?r=",iofs);
    if (id_srv != string::npos) iofs=id_srv+1;
    size_t id_name= URL.find("&cn=",iofs);
    int id_name_sz=4;
    if (string::npos == id_name){
        id_name= URL.find("&n=",iofs);
        id_name_sz=3;
    }
    std::string wwwAdr="";
    std::string realm="";
    std::string player="";
    if (id_name != string::npos){
        //extract url, realm and player names
        wwwAdr=URL.substr(0,id_folder);
        realm=URL.substr(id_srv+3,id_name-id_srv-3);
        size_t id_amp=URL.find("&",id_name+1);
        if (id_amp != string::npos)
            player=URL.substr(id_name+id_name_sz,id_amp-id_name-id_name_sz);
        else
        player=URL.substr(id_name+id_name_sz,URL.length()-id_name-id_name_sz);
    }else
        return false;
    if (string::npos == id_http) wwwAdr="http://"+wwwAdr;
    // retrieve armory gear data (XML) for this player
    URL= wwwAdr+"/character-sheet.xml?r="+realm+"&cn="+player;
    std::string src= getArmoryData(URL);
    if (src=="") return false;

    // parse that XML and search for available data
    // each recognized option add to optionStr
    std::string node;
    node= getValue(src,"character.class");
    if (parseName&&(node!="")){
        node[0]=tolower(node[0]);
        optionStr+= chkValue(src,"character.name",node+"=");
        optionStr+= chkValue(src,"character.level","level=");
    }

    if (parseGear){
        optionStr+= chkValue(src,"baseStats.strength.effective","gear_strength=");
        optionStr+= chkValue(src,"baseStats.agility.effective","gear_agility=");
        optionStr+= chkValue(src,"baseStats.stamina.effective","gear_stamina=");
        optionStr+= chkValue(src,"baseStats.intellect.effective","gear_intellect=");
        optionStr+= chkValue(src,"baseStats.spirit.effective","gear_spirit=");
        optionStr+= chkValue(src,"baseStats.armor.effective","gear_armor=");
        optionStr+= chkValue(src,"spell.manaRegen.casting","gear_mp5="); 

        optionStr+= chkMaxValue(src,"spell.bonusHealing.value","gear_spell_power="); //maybe max(all spells)
        optionStr+= chkMaxValue(src,"melee.power.effective,ranged.power.effective","gear_attack_power="); 
        optionStr+= chkMaxValue(src,"melee.expertise.rating","gear_expertise_rating="); 
        optionStr+= chkMaxValue(src,"pell.hitRating.penetration,melee.hitRating.penetration,ranged.hitRating.penetration","gear_armor_penetration_rating="); 
        optionStr+= chkMaxValue(src,"spell.hasteRating.hasteRating,melee.mainHandSpeed.hasteRating,ranged.speed.hasteRating","gear_haste_rating="); 
        optionStr+= chkMaxValue(src,"spell.hitRating.value,melee.hitRating.value,ranged.hitRating.value","gear_hit_rating="); 
        optionStr+= chkMaxValue(src,"spell.critChance.rating,melee.critChance.rating,ranged.critChance.rating","gear_crit_rating="); 

        optionStr+= chkValue(src,"characterBars.health.effective","gear_health=");
        node= getValue(src, "characterBars.secondBar.type");
        if (node=="m")
            optionStr+= chkValue(src,"characterBars.secondBar.effective","gear_mana=");
    }

    // parse options so far, in order to create player, because following parses may need to call player->parse
    if (optionStr!=""){
        if (debug) printf("%s", optionStr.c_str()); ;
        char* buffer= new char[optionStr.length()+20];
        strcpy(buffer,optionStr.c_str());
        option_t::parse_line( sim, buffer );
        delete buffer;
        optionStr="";
    };

    // parse for glyphs
    node = getNode(src, "characterTab.glyphs");
    if (node!=""){
        std::string glyph_node, glyph_name;
        for (int i=1; i<=3; i++){
            glyph_node= getNodeOne(node, "glyph",i);
			optionStr+= addItemGlyphOption(sim, glyph_node, "name", AEF_GLYPH);
        }
    }
    
    // parse items for effects, procs etc
    node = getNode(src, "characterTab.items");
    if (node!=""){
        std::string item_node, item_id;
        for (int i=1; i<=18; i++){
            item_node= getNodeOne(node, "item",i);
			optionStr+= addItemGlyphOption(sim, item_node, "id", AEF_ITEM);
			optionStr+= addItemGlyphOption(sim, item_node, "gem0Id", AEF_ITEM);
			optionStr+= addItemGlyphOption(sim, item_node, "gem1Id", AEF_ITEM);
			optionStr+= addItemGlyphOption(sim, item_node, "gem2Id", AEF_ITEM);
        }
    }



    // retrieve armory talents data (XML) for this player
    URL= wwwAdr+"/character-talents.xml?r="+realm+"&cn="+player;
    if (parseTalents){
        src= getArmoryData(URL);
        src= getNode(src, "talentGroups");
        node= getNode(src,"talentGroup");
        std::string active= getValue(node,"active");
        if (active!="1"){
            node= getNodeOne(src,"talentGroup",2);
        }
        optionStr+= chkValue(node, "talentSpec.value", "talents=http://worldofwarcraft?encoded=");

    }
    
    // save url caches
    SaveCache();

    // now parse remaining options
    if (optionStr!=""){
        if (debug) printf("%s", optionStr.c_str()); ;
        char* buffer= new char[optionStr.length()+20];
        strcpy(buffer,optionStr.c_str());
        option_t::parse_line( sim, buffer );
        delete buffer;
    }
    if (debug) {char c; scanf("%c",&c);}
    return true;
}


std::string checkItemGlyphOption(aef_type t, std::string id_name){
    armor_effect_t table[] =
    {
        { AEF_ITEM, "41285", "chaotic_skyflare=1" },
        { AEF_ITEM, "41333", "ember_skyflare=1" },
        { AEF_ITEM, "45308", "eye_of_the_broodmother=1" },
        { AEF_ITEM, "45518", "flare_of_the_heavens=1" },
        { AEF_ITEM, "40432", "illustration_of_the_dragon_soul=1" },
        { AEF_NONE, NULL, NULL}
    };
    return parseItemGlyphOption(table, 0, t, id_name);
}
