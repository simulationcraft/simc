// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simcraft.h"




// **************************************************************************
//
// ACT_EXPRESSION_T implementation
//
//   - base class for all expressions
//   - include most needed functionalities
//   - few descendant classes:
//         global_expression_t (to get functions/values from action,player,sim)
//         pbuff_expression_t  (to get buff expiration time)
//         oldbuff_expression_t  (to guess "old" buff expiration time)
//
// **************************************************************************

  // types of operations in expressions
  enum exp_type { AEXP_NONE=0, 
                  AEXP_AND, AEXP_OR, AEXP_NOT, AEXP_EQ, AEXP_NEQ, AEXP_GREATER, AEXP_LESS, AEXP_GE, AEXP_LE, // these operations result in boolean
                  AEXP_PLUS, AEXP_MINUS, AEXP_MUL, AEXP_DIV, // these operations result in double
                  AEXP_VALUE, AEXP_INT_PTR, AEXP_DOUBLE_PTR, // these are "direct" value return (no operations)
                  AEXP_BUFF, AEXP_FUNC, // these presume overriden class
                  AEXP_MAX
  };

  // types of functions and variables in BUFF and FUNC operators
  enum exp_func_glob { EFG_NONE=0, EFG_GCD, EFG_TIME, EFG_TTD, EFG_HP, EFG_VULN, EFG_INVUL, 
                       EFG_TICKING, EFG_CTICK,EFG_NTICKS, EFG_EXPIRE, EFG_REMAINS, EFG_TCAST, EFG_MOVING, EFG_MAX };

  // types of known prefixes in expressions
  enum exp_prefixes { EXPF_NONE=0, EXPF_BUFF, EXPF_DEBUFF, EXPF_GLOBAL, EXPF_ACTION, EXPF_THIS, 
                      EXPF_OPTION, EXPF_TALENT, EXPF_GLYPH, EXPF_ITEM, EXPF_GEAR, EXPF_MAX };



  // definition of operators
  struct operator_def_t{
    std::string name;
    int type;
    bool binary;
  };

  static operator_def_t AEXP_operators[]=
  {
    { "&&", AEXP_AND,     true},
    { "&",  AEXP_AND,     true},
    { "||", AEXP_OR,      true},
    { "|",  AEXP_OR,      true},
    { "!",  AEXP_NOT,     false},
    { "==", AEXP_EQ,      true},
    { "<>", AEXP_NEQ,     true},
    { ">=", AEXP_GE,      true},
    { "<=", AEXP_LE,      true},
    { ">",  AEXP_GREATER, true},
    { "<",  AEXP_LESS,    true},
    { "+",  AEXP_PLUS,    true},
    { "-",  AEXP_MINUS,   true},
    { "*",  AEXP_MUL,     true},
    { "/",  AEXP_DIV,     true},
    { "\\", AEXP_DIV,     true},
    { "",   AEXP_NONE,    false}
  };


  // oldbuff_expression_t:: custom class to calculate "old" buffs expirations
  oldbuff_expression_t::oldbuff_expression_t(std::string expression_str, void* value_ptr, event_t** e_expiration, int ptr_type)
                        :act_expression_t(AEXP_BUFF,expression_str,0) 
  {
    expiration_ptr=e_expiration;
    if (ptr_type=2){
      int_ptr=0;
      double_ptr=(double*) value_ptr;
    }else{
      int_ptr=(int*) value_ptr;
      double_ptr=0;
    }
  }

  double oldbuff_expression_t::evaluate() {
    double time=0;
    double value=0;
    // get current value
    if (int_ptr) value=*int_ptr;
    if (double_ptr) value=*double_ptr;
    // if this old buff do not have expirations, return values 
    if (expiration_ptr==0) return value; 
    // get time until expiration
    event_t* expiration=*expiration_ptr;
    if (expiration!=0){
      time= expiration->time;
      if (expiration->reschedule_time>time) 
        time= expiration->reschedule_time;
      time-= expiration->sim->current_time;
      if (time<0) time=0;
    }
    // if expiration is not up, check if buff is marked as up anyway
    if ((time==0)&&(value!=0)) time=0.1;
    return time;
  }

  // custom class to invoke pbuff_t expiration
  struct pbuff_expression_t: public act_expression_t{
    pbuff_t* buff;
    pbuff_expression_t(std::string expression_str="", pbuff_t* e_buff=0):act_expression_t(AEXP_BUFF,expression_str,0), buff(e_buff){};
    virtual ~pbuff_expression_t(){};
    virtual double evaluate() {
      return buff->expiration_time();
    }
  };

  //custom class to return global functions
  struct global_expression_t: public act_expression_t{
    action_t* action;
    int glob_type;
    global_expression_t(int e_type, std::string expression_str="", action_t* e_action=0):act_expression_t(AEXP_FUNC,expression_str,0), action(e_action), glob_type(e_type){};
    virtual ~global_expression_t(){};
    virtual double evaluate() {
      switch (glob_type){
        case EFG_GCD:        return action->gcd();  
        case EFG_TIME:       return action->sim->current_time; 
        case EFG_TTD:        return action->sim -> target ->time_to_die(); 
        case EFG_HP:         return action->sim -> target -> health_percentage();  
        case EFG_VULN:       return action->sim -> target ->vulnerable;  
        case EFG_INVUL:      return action->sim -> target ->invulnerable;  
        case EFG_TICKING:    return action->ticking;
        case EFG_CTICK:      return action->current_tick;
        case EFG_NTICKS:     return action->num_ticks;
        case EFG_EXPIRE:     return action->duration_ready;
        case EFG_REMAINS:    {
                                double rem=action->duration_ready-action->sim->current_time;
                                return rem>0? rem:0;
                             }
        case EFG_TCAST:      return action->execute_time();
        case EFG_MOVING:     return action->player->moving;
      }
      return 0;
    }
  };

  act_expression_t::act_expression_t(int e_type, std::string expression_str, double e_value)
  {
    type=e_type;
    operand_1=NULL;
    operand_2=NULL;
    p_value=NULL;
    value=e_value;
    exp_str=expression_str;
  }

  // evaluate expressions based on 2 or 1 operand
  double act_expression_t::evaluate()
  {
    switch (type){
      case AEXP_AND:        return operand_1->ok() && operand_2->ok();  
      case AEXP_OR:         return operand_1->ok() || operand_2->ok();  
      case AEXP_NOT:        return !operand_1->ok();  
      case AEXP_EQ:         return operand_1->evaluate() == operand_2->evaluate();  
      case AEXP_NEQ:        return operand_1->evaluate() != operand_2->evaluate();  
      case AEXP_GREATER:    return operand_1->evaluate() >  operand_2->evaluate();    
      case AEXP_LESS:       return operand_1->evaluate() <  operand_2->evaluate();  
      case AEXP_GE:         return operand_1->evaluate() >= operand_2->evaluate();    
      case AEXP_LE:         return operand_1->evaluate() <= operand_2->evaluate();  
      case AEXP_PLUS:       return operand_1->evaluate() +  operand_2->evaluate();  
      case AEXP_MINUS:      return operand_1->evaluate() -  operand_2->evaluate();  
      case AEXP_MUL:        return operand_1->evaluate() *  operand_2->evaluate();  
      case AEXP_DIV:        return operand_2->evaluate()? operand_1->evaluate()/  operand_2->evaluate() : 0 ;  
      case AEXP_VALUE:      return value;  
      case AEXP_INT_PTR:    return *((int*)p_value);  
      case AEXP_DOUBLE_PTR: return *((double*)p_value);  
    }
    return 0;
  }

  bool   act_expression_t::ok()
  {
    return evaluate()!=0; 
  }

  //handle warnings/eeor reporting, mainly in parse phase
  // Severity:  0= ignore warning
  //            1= just warning
  //            2= report error, but do not break
  //            3= breaking error, assert
void act_expression_t::warn(int severity, action_t* action, const char* format, ... )
  {
    char buffer[ 1024 ];
    va_list vap;
    va_start( vap, format );
    vsprintf( buffer, format, vap );

    std::string e_msg;
    if (severity<2)  e_msg="Warning";  else  e_msg="Error";
    e_msg+="("+action->name_str+"): "+ buffer;
    printf("%s\n", e_msg.c_str());
    if ( action->sim -> debug ) log_t::output( action->sim, "Exp.parser warning: %s", e_msg.c_str() );
    if (severity==3)
    {
      printf("Expression parser breaking error\n");
      assert(false);
    }
  }


  act_expression_t* act_expression_t::find_operator(action_t* action, std::string& unmasked, std::string& expression, std::string& op_str, 
                                                    int op_type, bool binary)
  {
    act_expression_t* node=0;
    size_t p=0;
    p=expression.find(op_str);
    if (p!=std::string::npos){
      std::string left =trim(unmasked.substr(0,p));
      std::string right=trim(unmasked.substr(p+op_str.length()));
      act_expression_t* op1= 0;
      act_expression_t* op2= 0;
      if (binary){
        op1= act_expression_t::create(action, left);
        op2= act_expression_t::create(action, right);
      }else{
        op1= act_expression_t::create(action, right);
        if (left!="") warn(2,action,"Unexpected text to the left of unary operator %s in : %s", op_str.c_str(), unmasked.c_str() );
      }
      //check allowed cases to miss left operand
      if ( (op1==0) && ((op_type==AEXP_PLUS)||(op_type==AEXP_MINUS)) ){
        op1= new act_expression_t(AEXP_VALUE,"",0);
      }
      // error handling
      if ((op1==0)||((op2==0)&&binary))
        warn(3,action,"Left or right operand for %s missing in : %s", op_str.c_str(), unmasked.c_str() );
      // create new node
      node= new act_expression_t(op_type,unmasked);
      node->operand_1=op1;
      node->operand_2=op2;
      // optimize if both operands are constants
      if ( (op1->type==AEXP_VALUE) && ((!binary)||(op2->type==AEXP_VALUE)) ){
        double val= node->evaluate();
        delete node;
        node= new act_expression_t(AEXP_VALUE,unmasked,val);
      };
      // optimize if one operand is constant, for boolean param ops: AEXP_AND, AEXP_OR
      if ((op1!=0)&&(op2!=0))
      if ( ((op1->type==AEXP_VALUE)||(op2->type==AEXP_VALUE)) &&  
           ((op_type==AEXP_AND)||(op_type==AEXP_OR)) ){
        // get constant value
        act_expression_t* nc=0;
        bool c_val=0;
        if (op1->type==AEXP_VALUE){
          c_val=op1->ok();
          nc=op2;
        }else{
          c_val=op2->ok();
          nc=op1;
        }
        // now select if it evaluate to constant
        delete node;
        if ( ((op_type==AEXP_AND)&&(c_val==false))||
             ((op_type==AEXP_OR) &&(c_val==true)) ) {
          node= new act_expression_t(AEXP_VALUE,unmasked,c_val);
          delete nc;
        }else{
          //or it evaluate to single operand
          node=nc;
          node->exp_str=unmasked;
        }
      }
    }
    return node;
  }

  // mark bracket "groups" as special chars (255..128, ie negative chars)
  std::string bracket_mask(std::string& src, std::string& dst){
    std::string err="";
    dst="";
    int b_lvl=0;
    char b_num=0;
    for (size_t p=0; p<src.length(); p++){
      char ch=src[p];
      if (ch=='('){
        b_lvl++;
        if (b_lvl==1) b_num--;
      }
      if (b_lvl>50) return "too many opening brackets!";
      if (b_lvl==0)
        dst+=ch;
      else
        dst+=b_num;
      if (ch==')') b_lvl--;
      if (b_lvl<0)  return "Too many closing brackets!";
    }
    if (b_lvl!=0) return "Brackets not closed!";
    return "";
  }


  // this parse expression string and create needed exp.tree
  // using different types of "expression nodes"
  act_expression_t* act_expression_t::create(action_t* action, std::string& expression)
  {
    act_expression_t* root=0;
    std::string e=trim(tolower(expression));
    if (e=="") return root;   // if empty expression, fail
    replace_char(e,'~','=');  // since options do not allow =, so ~ can be used instead
    replace_str(e,"!=","<>"); // since ! has to be searched before !=
    // process parenthesses
    std::string m_err, mask;
    bool is_all_enclosed;
    do{
      is_all_enclosed=false;
      m_err= bracket_mask(e,mask);
      if (m_err!="")  warn(3,action,"%s", m_err.c_str() );
      //remove enclosing parentheses if any
      if (mask.length()>1){
        int start_bracket= (mask[0]<0)||((unsigned char)mask[0]>240)?mask[0]:0;
        if (start_bracket && (mask[mask.length()-1]==start_bracket)){
          is_all_enclosed=true;
          e.erase(0,1);
          e.erase(e.length()-1);
          e=trim(e);
        }
      }
    } while (is_all_enclosed);

    // search for operators, starting with lowest priority operator
    for (int i=0; (AEXP_operators[i].type!=AEXP_NONE)&&(!root); i++)
      root=find_operator(action,e,mask, AEXP_operators[i].name, AEXP_operators[i].type, AEXP_operators[i].binary);

    // check if this is fixed value/number 
    if (!root){
      double val;
      if (str_to_float(e, val)){
        root= new act_expression_t(AEXP_VALUE,e,val);
      }
    }

    // search for "named value", if nothing above found
    if ((!root)&&(e!="")){
      std::vector<std::string> parts;
      unsigned int num_parts = util_t::string_split( parts, e, "." );
      if ((num_parts>3)||(num_parts==0)) warn(3,action,"Wrong number of parts(.) in : %s", e.c_str() );
      
      // check for known prefix (optional for now)
      exp_prefixes prefix=EXPF_NONE;
      std::string pfx_candidate=parts[0];
      if (pfx_candidate=="buff") prefix=EXPF_BUFF;
      if (pfx_candidate=="buffs") prefix=EXPF_BUFF;
      if (pfx_candidate=="debuff") prefix=EXPF_DEBUFF;
      if (pfx_candidate=="debuffs") prefix=EXPF_DEBUFF;
      if (pfx_candidate=="global") prefix=EXPF_GLOBAL;
      if (pfx_candidate=="action") prefix=EXPF_ACTION;
      if (pfx_candidate=="spell") prefix=EXPF_ACTION;
      if (pfx_candidate=="dot") prefix=EXPF_ACTION; 
      if (pfx_candidate=="this") prefix=EXPF_THIS;
      if (pfx_candidate=="option") prefix=EXPF_OPTION;
      if (pfx_candidate=="options") prefix=EXPF_OPTION;
      if (pfx_candidate=="talent") prefix=EXPF_OPTION; //for now, same as options
      if (pfx_candidate=="talents") prefix=EXPF_OPTION; 
      if (pfx_candidate=="glyph") prefix=EXPF_GLYPH;
      if (pfx_candidate=="item") prefix=EXPF_ITEM;
      if (pfx_candidate=="gear") prefix=EXPF_GEAR; 

      // get name of value, and suffix
      std::string name="";
      std::string suffix="";
      if (prefix==EXPF_NONE) pfx_candidate="";
      unsigned int p_name=prefix ? 1:0;
      if (p_name<num_parts)   name=parts[p_name]; 
      if (p_name+1<num_parts) suffix=parts[p_name+1]; 
      if (name=="")
        warn(3,action,"Wrong prefix.sufix combination for : %s", e.c_str() );

      // now search for name in categories
      if (name!=""){
        // Buffs
        if ( ((prefix==EXPF_BUFF)||(prefix==EXPF_NONE)) && !root) {
          pbuff_t* buff=action->player->buff_list.find_buff(name);
          bool sfx_stacks=false;
          if (suffix=="value")  sfx_stacks=true;
          if (suffix=="buff")   sfx_stacks=true;
          if (suffix=="stacks") sfx_stacks=true;
          // now set buff if found
          if (buff){
            if (!sfx_stacks){
              pbuff_expression_t* e_buff= new pbuff_expression_t(e,buff);
              root= e_buff;
            }else{
              root= new act_expression_t(AEXP_DOUBLE_PTR,e);
              root->p_value= &buff->buff_value;
            }
          }
        }
        // Debuffs, on target
        if ( (prefix==EXPF_DEBUFF) && !root) {
            // for now, hard-coded
            int* int_ptr=0;
            double* double_ptr=0;
            if (name=="isb")        int_ptr= &action->sim->target->debuffs.improved_shadow_bolt; 
            // create if found
            if (int_ptr!=0){
              root= new act_expression_t(AEXP_INT_PTR,e);
              root->p_value= int_ptr;
            }else
            if (double_ptr!=0){
              root= new act_expression_t(AEXP_DOUBLE_PTR,e);
              root->p_value= double_ptr;
            }
        }
        // actions, prefix either "cast"(0) or "this.gcd"(7) or "dot.immolate.remains"(6)
        if ( ((prefix==EXPF_ACTION)||(prefix==EXPF_THIS)||(prefix==EXPF_NONE)) && !root) {
          // find action in question
          action_t* act=0;
          std::string func_name="";
          if ((prefix==EXPF_THIS)||(prefix==EXPF_NONE)){ 
            act=action; 
            func_name=name;
          }else{
            func_name=suffix;
            for (action_t* p_act=action->player->action_list; p_act && !act; p_act=p_act->next){
              if (proper_option_name(name)==proper_option_name(p_act->name_str))
                act=p_act;
            }
          }
          if (!act) 
            warn(3,action,"Could not find action (%s) for %s", name.c_str(), e.c_str() );
          // now create execution node
          int func_type=0;
          if (func_name=="gcd") func_type=EFG_GCD;
          if (func_name=="active") func_type=EFG_TICKING;
          if (func_name=="ticking") func_type=EFG_TICKING;
          if (func_name=="current_tick") func_type=EFG_CTICK;
          if (func_name=="tick") func_type=EFG_CTICK;
          if (func_name=="num_ticks") func_type=EFG_NTICKS;
          if (func_name=="expire_time") func_type=EFG_EXPIRE;
          if (func_name=="expire") func_type=EFG_EXPIRE;
          if (func_name=="remains_time") func_type=EFG_REMAINS;
          if (func_name=="remains") func_type=EFG_REMAINS;
          if (func_name=="cast_time") func_type=EFG_TCAST;
          if (func_name=="execute_time") func_type=EFG_TCAST;
          if (func_name=="cast") func_type=EFG_TCAST;
          // now create node if known global value
          if (func_type>0){
            global_expression_t* g_func= new global_expression_t(func_type,e,act);
            root= g_func;
          }else
            if (prefix!=EXPF_NONE)
              warn(3,action,"Could not find value/function (%s) for %s", name.c_str(), e.c_str() );
        }
        // Global functions, player, target or sim related
        if ( ((prefix==EXPF_GLOBAL)||(prefix==EXPF_NONE)) && !root) {
          int glob_type=0;
          //first names that are not expressions, but option settings
          if (name=="allow_early_recast"){
            action->is_ifall=1;
            e="1";
          }
          // now regular names
          if (name=="time") glob_type=EFG_TIME;
          if (name=="time_to_die") glob_type=EFG_TTD;
          if (name=="health_percentage") glob_type=EFG_HP;
          if (name=="vulnerable") glob_type=EFG_VULN;
          if (name=="invulnerable") glob_type=EFG_INVUL;
          if (name=="moving") glob_type=EFG_MOVING;
          if (name=="move") glob_type=EFG_MOVING;
          // now create node if known global value
          if (glob_type>0){
            global_expression_t* g_func= new global_expression_t(glob_type,e,action);
            root= g_func;
          }
        }
        // Options(10), Glyphs(12), Items(13), Gear(14) - just existence, so "glyph.life_tap"
        if (  (!root) &&
              ((prefix==EXPF_OPTION)||(prefix==EXPF_GLYPH)||(prefix==EXPF_GEAR)||(prefix==EXPF_ITEM)) )
        {
          double item_value=0;
          bool item_found=false;
          if (((prefix==EXPF_GLYPH)||(prefix==EXPF_GEAR)) && !item_found) { //glyphs
            std::vector<std::string> glyph_names;
            int num_glyphs = util_t::string_split( glyph_names, action->player->glyphs_str, ",/" );
            for( int i=0; i < num_glyphs; i++ )
              if ( glyph_names[ i ] == name ){
                item_value=1;
                item_found=true;
              }
          };
          if (((prefix==EXPF_ITEM)||(prefix==EXPF_GEAR)) && !item_found){ //items
            for( int i=0; i < SLOT_MAX; i++ )
            {
              item_t& item = action->player->items[ i ];
              if( item.active()&& ( (item.encoded_name_str==name) || (strcmp(item.slot_name(),name.c_str())==0) ))
              {
                item_found=true;
                //if suffix present, return stat value, like "item.head.hit>35", but no need atm...
                int sfx_idx=0;
                if (suffix!=""){
                  for (int i=1; i<STAT_MAX; i++) 
                    if (tolower(util_t::stat_type_abbrev(i))==suffix)
                      sfx_idx=i;
                }
                if (sfx_idx)
                  item_value=item.stats.get_stat(sfx_idx);
                else
                  item_value=1;
              }
            }
          };
          if (!item_found)
          { //other options
            option_t* opt_found=0;
            //first look in player options
            for (unsigned int i=0; (i<action->player->options.size())&&!opt_found; i++){
              option_t& opt=action->player->options[i];
              if ((opt.name==name)&&(opt.address))   opt_found=&opt;
            }
            // if not found, check sim options
            if (!opt_found)
            for (unsigned int i=0; (i<action->sim->options.size())&&!opt_found; i++){
              option_t& opt=action->sim->options[i];
              if ((opt.name==name)&&(opt.address))   opt_found=&opt;
            }
            // now get value
            if (opt_found){
              item_found=true;
              switch ( opt_found->type )
              {
                case OPT_APPEND: 
                case OPT_STRING: if (""!=*( ( std::string* ) opt_found->address ))  item_value=1;      break;
                case OPT_BOOL:
                case OPT_INT:    item_value=*( ( int* ) opt_found->address ) ;                         break;
                case OPT_FLT:    item_value=*( ( double* ) opt_found->address ) ;                      break;
              }
            }
          };
          //create constant node with this. Value will be zero(false) if not found
          root= new act_expression_t(AEXP_VALUE,e,item_value);
        }
        // if nothing found so far, try function extensions
        if (!root){
          root= action->create_expression(name, pfx_candidate, suffix);
        }
      }

    }
    //return result for act_expression_t::create
    return root;
  }

  //return name of operand , based on type
  std::string act_expression_t::op_name(int op_type){
    if (op_type<=AEXP_NONE) return "NONE";
    if (op_type==AEXP_VALUE) return "VALUE";
    if (op_type==AEXP_INT_PTR) return "INT_PTR";
    if (op_type==AEXP_DOUBLE_PTR) return "DOUBLE_PTR";
    if (op_type==AEXP_BUFF) return "BUFF";
    if (op_type==AEXP_FUNC) return "VARIABLE";
    // now check list
    for (int i=0; (AEXP_operators[i].type!=AEXP_NONE); i++)
      if (AEXP_operators[i].type==op_type) 
        return AEXP_operators[i].name;
    //if none found
    return "Unknown operator!";
  }

  // fill expression into string, for debug purpose
  std::string act_expression_t::to_string(int level){
    std::string s="\n";
    std::string lvl_tab="";
    for (int i=0; i<level; i++) lvl_tab+="  ";
    char buff[100];
    sprintf(buff,"%d",level);
    s+=lvl_tab+"["+buff+"]:  "+op_name(type);
    if (type==AEXP_VALUE){
      sprintf(buff,"%1.2lf",value);
      s=s+"("+buff+") ";
    }
    s+="  : "+exp_str+"\n";
    if (operand_1) s+=operand_1->to_string(level+1);
    if (operand_2) s+=operand_2->to_string(level+1);
    return s;
  }



  










// **************************************************************************
//
// PBUFF_T  implementation
//
//    - encapsulate all functions for time duraction effects like buffs
//    - replace or remove need for previously used structures/functions:
//        _buff_t::int buff_XYZ_variable;
//        _cooldowns_t::buff_XYZ_cooldown
//        _expirations_t::event_t* buff_XYZ_expiration;
//        uptime_t* uptimes_buff_XYZ;
//        rng_t* rng_trigger_buff_XYZ;
//        static void trigger_buff_XYZ( spell_t* s ) {..~40 lines ...}
//    - above declarations and triggers are replaced with:
//        pbuff_t* buff_XYZ;        
//        buff_XYZ->trigger();
//
//    - allows settings of "value" that buff would have if active, so
//      usual usage (possible at N places) can also be reduced from:
//        if ( p -> _buffs.pyroclasm ) 
//           player_multiplier *= 1.0 + p -> talents.pyroclasm * 0.02;
//        p -> uptimes_pyroclasm -> update( p -> _buffs.pyroclasm != 0 );
//    - to something like:
//        player_multiplier *= p->pyroclasm->mul_value();
//    - or it also can use IF to do more complex stuff, but no need to update uptimes:
//        if (p->pyroclasm->is_up())
//           player_multiplier *= 1.0 + p -> talents.pyroclasm * 0.02;
//
// **************************************************************************



  pbuff_t::pbuff_t(player_t* plr, std::string name, double duration, double cooldown, int aura_idx, double use_value, bool t_ignore, double t_chance ){
    player=plr;
    name_str=name;
    name_expiration = name_str+"_Expiration";
    buff_value=0;
    aura_id=aura_idx;
    buff_duration=duration;
    buff_cooldown=cooldown;
    ignore=t_ignore;
    chance=t_chance;
    rng_chance=0;
    value=use_value;
    expiration=0;
    last_trigger=0;
    expected_end=0;
    next=0;
    n_triggers=0;
    n_trg_tries=0;
    callback_expiration=NULL;
    trigger_counter=NULL;
    if (chance!=0){
      int rng_type=RNG_DISTRIBUTED;
      // if "negative" chance, use simple random
      if (chance<0){
        chance=-chance;
        rng_type=RNG_CYCLIC;
      }
      rng_chance   = player->get_rng( name+"_buff",   rng_type );
    }
    uptime_cnt= player->get_uptime(name); 
    be_silent=false;
    player->buff_list.add_buff(this);
  }
  //reset buff, called at end of iteration  
  void pbuff_t::reset(){
    expiration=0;
    last_trigger=0;
    expected_end=0;
    buff_value=0; 
    uptime_cnt->rewind();
  }
  // trigger buff if conditions are met (return false otherwise)
  // will use duration and chance from buff constructor if none supplied here
  bool pbuff_t::trigger(double val, double b_duration,int aura_idx){
    n_trg_tries++;
    // if talents or other reasons do not allow this buff, skip it
    if (ignore) return false;
    // check cooldown if any
    if (player->sim->current_time <= last_trigger+buff_cooldown) return false; 
    // if chance is supplied, check it
    if (chance>0) 
        if ( !rng_chance -> roll( chance ) ) return false;
    // if we passed all checks, trigger buff
    n_triggers++;
    uptime_cnt->n_triggers= n_triggers;
    last_trigger=player->sim->current_time;
    if (trigger_counter) *trigger_counter++;
    // set value and update counters
    buff_value=val;
    uptime_cnt -> update( 1, true );
    // create and launch expiration event
    if (b_duration==0) b_duration=buff_duration;
    expected_end=last_trigger+b_duration;
    if ( expiration )
    {
      expiration -> reschedule( b_duration );
      expiration->n_trig= n_triggers;
    }
    else
    {
      expiration = new (player->sim) buff_expiration_t( this, b_duration, aura_idx );
    } 
    return true;
  }
  //return time till expiration (or 0 if expired). Used as function pointer in expressions
  double  pbuff_t::expiration_time()
  {
    if (!is_up_silent()) return 0; 
    double remains= expected_end - player->sim->current_time;
    if (remains<0) remains=0;
    if ((remains<=0)&& is_up_silent())
      remains=1; // fixing when buff is set outside without triggers - will report as 1sec remaining
    return remains;
  }


  // check if buff is up, without updating counters
  bool pbuff_t::is_up_silent(){
    return (buff_value!=0);
  }
  // if buff is up, return "value", otherwise return 1
  double pbuff_t::mul_value_silent()
  {
    if (is_up_silent())
      return value;
    else
      return 1;
  }
  // if buff is up, return "value", otherwise return 0
  double pbuff_t::add_value_silent()
  {
    if (is_up_silent())
      return value;
    else
      return 0;
  }
  // update uptime counters
  void pbuff_t::update_uptime(bool skip_usage)
  {
    uptime_cnt -> update( is_up_silent(), skip_usage ); 
  }
  // decrement buff stack for one, return if buff still up
  bool pbuff_t::dec_buff(){
    if (buff_value>=1){
      buff_value--;
      if (expiration&&!is_up_silent()){
        //event_t::early(expiration);
        expiration -> canceled = 1; 
        expiration -> execute(); 
        expiration=0;
        update_uptime(true);
      }
    }
    return is_up_silent();
  }

  // now same versions that do change uptime counters
  bool pbuff_t::is_up(){
    update_uptime(be_silent);
    return is_up_silent();
  }
  double pbuff_t::mul_value(){
    update_uptime(be_silent);
    return mul_value_silent();

  }
  double pbuff_t::add_value(){
    update_uptime(be_silent);
    return add_value_silent();
  }


  // 
  // buff_expiration_t  implementation

  buff_expiration_t::buff_expiration_t( pbuff_t* p_buff, double b_duration, int aura_idx ) 
    : event_t( p_buff->player->sim, p_buff->player )
  {
    pbuff=p_buff;
    aura_id=aura_idx;
    if (aura_id==0) aura_id= pbuff->aura_id;
    player -> aura_gain( pbuff->name_expiration.c_str(), aura_id);
    n_trig= pbuff->n_triggers;
    sim -> add_event( this, b_duration );
  }

  void buff_expiration_t::execute()
  {
    if (pbuff->n_triggers== n_trig){
      player -> aura_loss( pbuff->name_str.c_str(), aura_id );
      pbuff->buff_value=0;
      pbuff->expected_end=0;
      pbuff->uptime_cnt -> update( 0, true );
      if (pbuff->trigger_counter) *pbuff->trigger_counter--;
      if (pbuff->callback_expiration) pbuff->callback_expiration();
    }else{
      //new buff was casted while old one was expiring
    }
    pbuff->expiration=0;
  }

  // 
  // buff_list_t  implementation

  buff_list_t::buff_list_t(): n_buffs(0),first(0)
  {
  }

  void buff_list_t::add_buff(pbuff_t* new_buff)
  {
    new_buff->next=first;
    first= new_buff;
    n_buffs++;
  }

  void buff_list_t::reset_buffs()
  {
    for (pbuff_t* p=first; p; p=p->next){
      p->reset();
    }
      
  }

  pbuff_t* buff_list_t::find_buff(std::string& name){
    pbuff_t* res=0;
    std::string prop_name=proper_option_name(name);
    for (pbuff_t* p=first; p && !res; p=p->next){
      if (proper_option_name(p->name_str)==prop_name)
        res=p;
    }
    return res;
  }

  bool buff_list_t::chk_buff(std::string& name){
    pbuff_t* buff= find_buff(name);
    if (buff) 
      return buff->is_up_silent();
    else
      return false;
  }






// **************************************************************************
//
// UPTIME_T  implementation
//
// **************************************************************************


uptime_t::uptime_t( const std::string& n, sim_t* the_sim) : name_str( n ), up( 0 ), down( 0 ), 
                    type(0), sim(the_sim), last_check(0), total_time(0),last_status(false), up_time(0), n_rewind(0), n_up(0), n_down(0), n_triggers(0),
                    avg_up(0), avg_dur(0)
{ 
}

void uptime_t::rewind()
{
  n_rewind++;
  if (last_status&&(n_up>n_down)) n_down++;
  last_status=false;
  last_check=0;
}

void   uptime_t::update( bool is_up, bool skip_usage ) 
{ 
  // this is "duration, time based" statistics
  if (sim){
    double t_span= sim->current_time - last_check;
    // check if rewind (back more than 80% of max_time)
    if ((t_span<0)&&(fabs(t_span) > sim->max_time*0.8)){
      rewind();
      t_span=sim->current_time;
    }
    //now process positive span
    if (t_span>=0){
      if (last_status) 
        up_time+= t_span;
      total_time+= t_span;
      if (is_up&&!last_status) n_up++;
      if (last_status&&!is_up) n_down++;
    }
    //store new values
    last_check=sim->current_time;
    last_status=is_up;
  }
  // this is "on use" statistics
  if (!skip_usage){
    if ( is_up ) 
      up++; 
    else 
      down++; 
  }
}

//return uptime statistics (not all are percentages)
double uptime_t::percentage(int p_type) 
{ 
  //calculate result for default, usage based
  double p_usage=( up==0 ) ? 0 : ( 100.0*up/( up+down ) );
  // calculate for other types
  if (n_down<n_up/4) n_down=n_up; // for those never turned off
  avg_up= (n_up+n_down)/2.0;
  if (avg_up>0) avg_dur=up_time/avg_up;
  if (n_rewind>0){
    avg_up/=n_rewind;
    if (avg_dur> total_time/n_rewind)
      avg_dur=total_time/n_rewind;
  }
  double p_time=(total_time==0)? 0 : (100.0* up_time/total_time);
  double buff_triggers= n_rewind>0?n_triggers/n_rewind: n_triggers;
  // return result
  switch (p_type){
    case 1: return p_usage;       break;
    case 2: return p_time;        break;
    case 3: return avg_dur;       break;
    case 4: return avg_up;        break;
    case 5: return buff_triggers; break;
  }
  //default result
  return p_usage;
}

void   uptime_t::merge( uptime_t* other ) 
{ 
  up_time+=other->up_time;
  total_time+=other->total_time;
  up += other -> up; 
  down += other -> down; 
}

const char* uptime_t::name() 
{ 
  return name_str.c_str(); 
}


