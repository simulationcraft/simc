// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.h"

std::string bracket_mask( std::string& src, std::string& dst );


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
//   - there are two phases:
//         creation with parse , by calling act_expression_t::create()
//         evaluation , by calling act_expression_t::evaluate() or ok()
//   - expressions can be extended via action/player->create_expression
//
// **************************************************************************


// types of functions and variables in BUFF and FUNC operators
enum exp_func_glob { EFG_NONE=0, EFG_GCD, EFG_TIME, EFG_TTD, EFG_HP, EFG_VULN, EFG_INVUL, EFG_COMBAT,
                     EFG_TICKING, EFG_CTICK,EFG_NTICKS, EFG_EXPIRE, EFG_REMAINS, EFG_TCAST, EFG_MOVING, EFG_DISTANCE, 
                     EFG_MANA, EFG_RAGE, EFG_ENERGY, EFG_FOCUS, EFG_RUNIC, EFG_RESOURCE,
                     EFG_MANA_PERC, EFG_RAGE_PERC, EFG_ENERGY_PERC, EFG_FOCUS_PERC, EFG_RUNIC_PERC, EFG_RESOURCE_PERC, EFG_MAX
                   };

// types of known prefixes in expressions
enum exp_prefixes { EXPF_NONE=0, EXPF_BUFF, EXPF_DEBUFF, EXPF_GLOBAL, EXPF_ACTION, EXPF_THIS,
                    EXPF_OPTION, EXPF_TALENT, EXPF_GLYPH, EXPF_ITEM, EXPF_GEAR, EXPF_PLAYER, EXPF_UNKNOWN, EXPF_MAX
                  };


static operator_def_t AEXP_operators[]=
{
  { "&&", AEXP_AND,     true  ,ETP_BOOL, ETP_BOOL},
  { "&",  AEXP_AND,     true  ,ETP_BOOL, ETP_BOOL},
  { "||", AEXP_OR,      true  ,ETP_BOOL, ETP_BOOL},
  { "|",  AEXP_OR,      true  ,ETP_BOOL, ETP_BOOL},
  { "!",  AEXP_NOT,     false ,ETP_BOOL, ETP_BOOL},
  { "==", AEXP_EQ,      true  ,ETP_BOOL, ETP_NUM },
  { "<>", AEXP_NEQ,     true  ,ETP_BOOL, ETP_NUM },
  { ">=", AEXP_GE,      true  ,ETP_BOOL, ETP_NUM },
  { "<=", AEXP_LE,      true  ,ETP_BOOL, ETP_NUM },
  { ">",  AEXP_GREATER, true  ,ETP_BOOL, ETP_NUM },
  { "<",  AEXP_LESS,    true  ,ETP_BOOL, ETP_NUM },
  { "+",  AEXP_PLUS,    true  ,ETP_NUM , ETP_NUM },
  { "-",  AEXP_MINUS,   true  ,ETP_NUM , ETP_NUM },
  { "*",  AEXP_MUL,     true  ,ETP_NUM , ETP_NUM },
  { "/",  AEXP_DIV,     true  ,ETP_NUM , ETP_NUM },
  { "\\", AEXP_DIV,     true  ,ETP_NUM , ETP_NUM },
  { "",   AEXP_NONE,    false ,ETP_BOOL, ETP_BOOL},
};


// oldbuff_expression_t:: custom class to calculate "old" buffs expirations
oldbuff_expression_t::oldbuff_expression_t( std::string expression_str, void* value_ptr, event_t** e_expiration, int ptr_type )
    :act_expression_t( AEXP_BUFF,expression_str,0 )
{
  expiration_ptr=e_expiration;
  if ( ptr_type==2 )
  {
    int_ptr=0;
    double_ptr=( double* ) value_ptr;
  }
  else
  {
    int_ptr=( int* ) value_ptr;
    double_ptr=0;
  }
}

double oldbuff_expression_t::evaluate()
{
  double time=0;
  double value=0;
  // get current value
  if ( int_ptr ) value=*int_ptr;
  if ( double_ptr ) value=*double_ptr;
  // if this old buff do not have expirations, return values
  if ( expiration_ptr==0 ) return value;
  // get time until expiration
  event_t* expiration=*expiration_ptr;
  if ( expiration!=0 )
  {
    time= expiration->time;
    if ( expiration->reschedule_time>time )
      time= expiration->reschedule_time;
    time-= expiration->sim->current_time;
    if ( time<0 ) time=0;
  }
  // if expiration is not up, check if buff is marked as up anyway
  if ( ( time==0 )&&( value!=0 ) ) time=0.1;
  return time;
}

// custom class to invoke pbuff_t expiration
struct pbuff_expression_t: public act_expression_t
{
  buff_t* buff;
  action_t* action;
  int method;
  pbuff_expression_t( std::string expression_str, buff_t* e_buff, int e_method, action_t* e_action=0 ):act_expression_t( AEXP_BUFF,expression_str,0 ),
      buff( e_buff ), action( e_action ), method( e_method ) {};
  virtual ~pbuff_expression_t() {};
  virtual double evaluate()
  {
    switch ( method )
    {
    case 1:
      // "buffname" or "buffname.duration<3" or "buffname.time>5"
      return buff->remains();
    case 2:
      // buffname.time_to_think
      return buff->may_react();
    case 3:
      // buffname.time_to_execute
      return buff->remains_gt( action->execute_time() );
    case 4:
      // buffname.time_to_do  ( #2 && #3 )
      return ( buff->may_react() && buff->remains_gt( action->execute_time() ) );
    }
    return buff->current_value;
  }
};

//custom class to return global functions
struct global_expression_t: public act_expression_t
{
  action_t* action;
  int glob_type;
  global_expression_t( int e_type, std::string expression_str="", action_t* e_action=0 ):act_expression_t( AEXP_FUNC,expression_str,0 ), action( e_action ), glob_type( e_type ) {};
  virtual ~global_expression_t() {};
  virtual double evaluate()
  {
    switch ( glob_type )
    {
    case EFG_GCD:        return action->gcd();
    case EFG_TIME:       return action->sim->current_time;
    case EFG_TTD:        return action->sim->target -> time_to_die();
    case EFG_HP:         return action->sim->target -> health_percentage();
    case EFG_VULN:       return action->sim->target -> debuffs.vulnerable -> check();
    case EFG_INVUL:      return action->sim->target -> debuffs.invulnerable -> check();
    case EFG_COMBAT:     return action->player->in_combat;
    case EFG_TICKING:    return action->ticking;
    case EFG_CTICK:      return action->current_tick;
    case EFG_NTICKS:     return action->num_ticks;
    case EFG_EXPIRE:     return action->duration_ready;
    case EFG_REMAINS:
    {
      double rem=action->duration_ready-action->sim->current_time;
      return rem>0? rem:0;
    }
    case EFG_TCAST:      return action->execute_time();
    case EFG_MOVING:     return action->player->buffs.moving->check();
    case EFG_DISTANCE:   return action->player->distance;
    case EFG_MANA:       return action->player->resource_current[ RESOURCE_MANA                      ];
    case EFG_RAGE:       return action->player->resource_current[ RESOURCE_RAGE                      ];
    case EFG_ENERGY:     return action->player->resource_current[ RESOURCE_ENERGY                    ];
    case EFG_FOCUS:      return action->player->resource_current[ RESOURCE_FOCUS                     ];
    case EFG_RUNIC:      return action->player->resource_current[ RESOURCE_RUNIC                     ];
    case EFG_RESOURCE:   return action->player->resource_current[ action->player->primary_resource() ];
    case EFG_MANA_PERC:
    {
      double max_mana=action->player->resource_max[ RESOURCE_MANA ];
      if ( max_mana<=0 ) return 0;
      return action->player->resource_current[ RESOURCE_MANA ]/max_mana*100;
    }
    case EFG_RAGE_PERC:
    {
      double max_rage=action->player->resource_max[ RESOURCE_RAGE ];
      if ( max_rage<=0 ) return 0;
      return action->player->resource_current[ RESOURCE_RAGE ]/max_rage*100;
    }
    case EFG_ENERGY_PERC:
    {
      double max_energy=action->player->resource_max[ RESOURCE_ENERGY ];
      if ( max_energy<=0 ) return 0;
      return action->player->resource_current[ RESOURCE_ENERGY ]/max_energy*100;
    }
    case EFG_FOCUS_PERC:
    {
      double max_focus=action->player->resource_max[ RESOURCE_FOCUS ];
      if ( max_focus<=0 ) return 0;
      return action->player->resource_current[ RESOURCE_FOCUS ]/max_focus*100;
    }
    case EFG_RUNIC_PERC:
    {
      double max_runic=action->player->resource_max[ RESOURCE_RUNIC ];
      if ( max_runic<=0 ) return 0;
      return action->player->resource_current[ RESOURCE_RUNIC ]/max_runic*100;
    }
    case EFG_RESOURCE_PERC:
    {
      double max_resource=action->player->resource_max[ action->player->primary_resource() ];
      if ( max_resource<=0 ) return 0;
      return action->player->resource_current[ action->player->primary_resource() ]/max_resource*100;
    }
    }
    return 0;
  }
};


// custom class to optimize AND
struct op_and_expression_t: public act_expression_t
{
  op_and_expression_t( std::string expression_str ):act_expression_t( AEXP_AND,expression_str,0 ) {};
  virtual ~op_and_expression_t() {};
  virtual double evaluate()
  {
    if ( !operand_1->ok() )
      return 0;
    else
      return  operand_2->ok();
  }
};

// custom class to optimize OR
struct op_or_expression_t: public act_expression_t
{
  op_or_expression_t( std::string expression_str ):act_expression_t( AEXP_OR,expression_str,0 ) {};
  virtual ~op_or_expression_t() {};
  virtual double evaluate()
  {
    if ( operand_1->ok() )
      return 1;
    else
      return  operand_2->ok();
  }
};

// custom class to optimize NOT
struct op_not_expression_t: public act_expression_t
{
  op_not_expression_t( std::string expression_str ):act_expression_t( AEXP_NOT,expression_str,0 ) {};
  virtual ~op_not_expression_t() {};
  virtual double evaluate()
  {
    return  !operand_1->ok();
  }
};


// act_expression_t  methods ============================================================

act_expression_t::act_expression_t( int e_type, std::string expression_str, double e_value )
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
  switch ( type )
  {
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
  case AEXP_INT_PTR:    return *( ( int* )p_value );
  case AEXP_DOUBLE_PTR: return *( ( double* )p_value );
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
void act_expression_t::warn( int severity, action_t* action, const char* format, ... )
{
  char buffer[ 1024 ];
  va_list vap;
  va_start( vap, format );
  vsprintf( buffer, format, vap );

  std::string e_msg;
  if ( severity<2 )  e_msg="Warning";  else  e_msg="Error";
  e_msg+="("+action->name_str+"): "+ buffer;
  util_t::printf( "%s\n", e_msg.c_str() );
  if ( action->sim -> debug ) log_t::output( action->sim, "Exp.parser warning: %s", e_msg.c_str() );
  if ( severity==3 )
  {
    util_t::printf( "Expression parser breaking error\n" );
    assert( 0 );
  }
}


act_expression_t* act_expression_t::find_operator( action_t* action, std::string& unmasked, std::string& expression, std::string& alias_protect,
                                                   operator_def_t& op )
{
  act_expression_t* node=0;
  size_t p=0;
  p=expression.find( op.name );
  if ( p!=std::string::npos )
  {
    std::string left =trim( unmasked.substr( 0,p ) );
    std::string right=trim( unmasked.substr( p+op.name.length() ) );
    act_expression_t* op1= 0;
    act_expression_t* op2= 0;
    if ( op.binary )
    {
      op1= act_expression_t::create( action, left  ,alias_protect, op.operand_type );
      op2= act_expression_t::create( action, right ,alias_protect, op.operand_type );
    }
    else
    {
      op1= act_expression_t::create( action, right ,alias_protect, op.operand_type );
      if ( left!="" ) warn( 2,action,"Unexpected text to the left of unary operator %s in : %s", op.name.c_str(), unmasked.c_str() );
    }
    //check allowed cases to miss left operand
    if ( ( op1==0 ) && ( ( op.type==AEXP_PLUS )||( op.type==AEXP_MINUS ) ) )
    {
      op1= new act_expression_t( AEXP_VALUE,"",0 );
    }
    // error handling
    if ( op1==0 )
      warn( 3,action,"First operand missing for %s in : %s", op.name.c_str(), unmasked.c_str() );
    if ( ( op2==0 )&&op.binary )
      warn( 3,action,"Second operand missing for %s in : %s", op.name.c_str(), unmasked.c_str() );
    // create new node, check if there are optimized sub-classes
    if ( op.type==AEXP_AND ) node= new op_and_expression_t( unmasked ); else
        if ( op.type==AEXP_OR )  node= new op_or_expression_t( unmasked ); else
            if ( op.type==AEXP_NOT ) node= new op_not_expression_t( unmasked ); else
          node= new act_expression_t( op.type,unmasked );
    node->operand_1=op1;
    node->operand_2=op2;
    // optimize if both operands are constants
    if ( ( op1->type==AEXP_VALUE ) && ( ( !op.binary )||( op2->type==AEXP_VALUE ) ) )
    {
      double val= node->evaluate();
      delete node;
      node= new act_expression_t( AEXP_VALUE,unmasked,val );
    };
    // optimize if one operand is constant, for boolean param ops: AEXP_AND, AEXP_OR
    if ( ( op1!=0 )&&( op2!=0 ) )
      if ( ( ( op1->type==AEXP_VALUE )||( op2->type==AEXP_VALUE ) ) &&
           ( ( op.type==AEXP_AND )||( op.type==AEXP_OR ) ) )
      {
        // get constant value
        act_expression_t* nc=0;
        bool c_val=0;
        if ( op1->type==AEXP_VALUE )
        {
          c_val=op1->ok();
          nc=op2;
        }
        else
        {
          c_val=op2->ok();
          nc=op1;
        }
        // now select if it evaluate to constant
        delete node;
        if ( ( ( op.type==AEXP_AND )&&( c_val==false ) )||
             ( ( op.type==AEXP_OR ) &&( c_val==true ) ) )
        {
          node= new act_expression_t( AEXP_VALUE,unmasked,c_val );
          delete nc;
        }
        else
        {
          //or it evaluate to single operand
          node=nc;
          node->exp_str=unmasked;
        }
      }
  }
  return node;
}

// mark bracket "groups" as special chars (255..128, ie negative chars)
std::string bracket_mask( std::string& src, std::string& dst )
{
  std::string err="";
  dst="";
  int b_lvl=0;
  char b_num=0;
  for ( size_t p=0; p<src.length(); p++ )
  {
    char ch=src[p];
    if ( ch=='(' )
    {
      b_lvl++;
      if ( b_lvl==1 ) b_num--;
    }
    if ( b_lvl>50 ) return "too many opening brackets!";
    if ( b_lvl==0 )
      dst+=ch;
    else
      dst+=b_num;
    if ( ch==')' ) b_lvl--;
    if ( b_lvl<0 )  return "Too many closing brackets!";
  }
  if ( b_lvl!=0 ) return "Brackets not closed!";
  return "";
}


// this parse expression string and create needed exp.tree
// using different types of "expression nodes"
act_expression_t* act_expression_t::create( action_t* action, std::string& expression, std::string alias_protect, exp_res_t expected_type )
{
  act_expression_t* root=0;
  std::string e=trim( tolower( expression ) );
  if ( e=="" ) return root; // if empty expression, fail
  replace_char( e,'~','=' );  // since options do not allow =, so ~ can be used instead
  replace_str( e,"!=","<>" ); // since ! has to be searched before !=
  // process parenthesses
  std::string m_err, mask;
  bool is_all_enclosed;
  do
  {
    is_all_enclosed=false;
    m_err= bracket_mask( e,mask );
    if ( m_err!="" )  warn( 3,action,"%s", m_err.c_str() );
    //remove enclosing parentheses if any
    if ( mask.length()>1 )
    {
      int start_bracket= ( mask[0]<0 )||( ( unsigned char )mask[0]>240 )?mask[0]:0;
      if ( start_bracket && ( mask[mask.length()-1]==start_bracket ) )
      {
        is_all_enclosed=true;
        e.erase( 0,1 );
        e.erase( e.length()-1 );
        e=trim( e );
      }
    }
  }
  while ( is_all_enclosed );

  // search for operators, starting with lowest priority operator
  for ( int i=0; ( AEXP_operators[i].type!=AEXP_NONE )&&( !root ); i++ )
    root=find_operator( action,e,mask, alias_protect, AEXP_operators[i] );

  // check if this is fixed value/number
  if ( !root )
  {
    double val;
    if ( str_to_float( e, val ) )
    {
      root= new act_expression_t( AEXP_VALUE,e,val );
    }
  }

  // search for "named value", if nothing above found
  if ( ( !root )&&( e!="" ) )
  {
    std::vector<std::string> parts;
    unsigned int num_parts = util_t::string_split( parts, e, "." );
    if ( ( num_parts>3 )||( num_parts==0 ) ) warn( 3,action,"Wrong number of parts(.) in : %s", e.c_str() );

    // check for known prefix
    // prexif is optional, but in order to differentiate 2 part conditions
    // "buff.pyroclasm"  vs "pyroclasm.duration"
    exp_prefixes prefix=EXPF_NONE;
    std::string pfx_candidate=parts[0];
    if ( pfx_candidate=="buff" ) prefix=EXPF_BUFF;
    if ( pfx_candidate=="buffs" ) prefix=EXPF_BUFF;
    if ( pfx_candidate=="debuff" ) prefix=EXPF_DEBUFF;
    if ( pfx_candidate=="debuffs" ) prefix=EXPF_DEBUFF;
    if ( pfx_candidate=="global" ) prefix=EXPF_GLOBAL;
    if ( pfx_candidate=="action" ) prefix=EXPF_ACTION;
    if ( pfx_candidate=="spell" ) prefix=EXPF_ACTION;
    if ( pfx_candidate=="dot" ) prefix=EXPF_ACTION;
    if ( pfx_candidate=="this" ) prefix=EXPF_THIS;
    if ( pfx_candidate=="option" ) prefix=EXPF_OPTION;
    if ( pfx_candidate=="options" ) prefix=EXPF_OPTION;
    if ( pfx_candidate=="talent" ) prefix=EXPF_OPTION; //for now, same as options
    if ( pfx_candidate=="talents" ) prefix=EXPF_OPTION;
    if ( pfx_candidate=="glyph" ) prefix=EXPF_GLYPH;
    if ( pfx_candidate=="item" ) prefix=EXPF_ITEM;
    if ( pfx_candidate=="gear" ) prefix=EXPF_GEAR;
    if ( pfx_candidate=="player" ) prefix=EXPF_PLAYER;

    // is this unknown prefix? can be used for extensions
    if ( ( prefix==EXPF_NONE )&&( num_parts==3 ) )  prefix=EXPF_UNKNOWN;
    // get name of value, and suffix
    std::string name="";
    std::string suffix="";
    if ( prefix==EXPF_NONE ) pfx_candidate="";
    unsigned int p_name=prefix ? 1:0;
    if ( p_name<num_parts )   name=parts[p_name];
    if ( p_name+1<num_parts ) suffix=parts[p_name+1];
    if ( name=="" )
      warn( 3,action,"Wrong prefix.sufix combination for : %s", e.c_str() );

    // now search for name in categories
    if ( name!="" )
    {
      // Buffs
      if ( ( ( prefix==EXPF_BUFF )||( prefix==EXPF_NONE ) ) && !root )
      {
        buff_t* buff=buff_t::find( action->player,name );
        // now set buff if found
        if ( buff )
        {
          bool sfx_stacks=( suffix=="value" )||( suffix=="buff" ) || ( suffix=="stacks" );
          if ( ( suffix=="" )&&( expected_type==ETP_BOOL ) ) sfx_stacks=true;
          if ( sfx_stacks )
          {
            int method=1;
            if ( ( suffix=="time_to_think" )||( suffix=="think" ) ) method=2;
            if ( suffix=="time_to_execute" ) method=3;
            if ( suffix=="time_to_do" ) method=4; // 2 && 3
            pbuff_expression_t* e_buff= new pbuff_expression_t( e,buff,method,action );
            root= e_buff;
          }
          else
          {
            root= new act_expression_t( AEXP_DOUBLE_PTR,e );
            root->p_value= &buff->current_value;
          }
        }
      }
      // Debuffs, on target
      if ( ( prefix==EXPF_DEBUFF ) && !root )
      {
        // for now, hard-coded
        int* int_ptr=0;
        double* double_ptr=0;
        if ( name=="isb" )        int_ptr= &action->sim->target->debuffs.improved_shadow_bolt->current_stack;
        // create if found
        if ( int_ptr!=0 )
        {
          root= new act_expression_t( AEXP_INT_PTR,e );
          root->p_value= int_ptr;
        }
        else
          if ( double_ptr!=0 )
          {
            root= new act_expression_t( AEXP_DOUBLE_PTR,e );
            root->p_value= double_ptr;
          }
      }
      // actions, prefix either "cast"(0) or "this.gcd"(7) or "dot.immolate.remains"(6)
      if ( ( ( prefix==EXPF_ACTION )||( prefix==EXPF_THIS )||( prefix==EXPF_NONE ) ) && !root )
      {
        // find action in question
        action_t* act=0;
        std::string func_name="";
        if ( ( prefix==EXPF_THIS )||( prefix==EXPF_NONE ) )
        {
          act=action;
          func_name=name;
        }
        else
        {
          func_name=suffix;
          for ( action_t* p_act=action->player->action_list; p_act && !act; p_act=p_act->next )
          {
            if ( proper_option_name( name )==proper_option_name( p_act->name_str ) )
              act=p_act;
          }
        }
        if ( !act )
          warn( 3,action,"Could not find action (%s) for %s", name.c_str(), e.c_str() );
        // now create execution node
        int func_type=0;
        if ( func_name=="gcd" ) func_type=EFG_GCD;
        if ( func_name=="active" ) func_type=EFG_TICKING;
        if ( func_name=="ticking" ) func_type=EFG_TICKING;
        if ( func_name=="current_tick" ) func_type=EFG_CTICK;
        if ( func_name=="tick" ) func_type=EFG_CTICK;
        if ( func_name=="num_ticks" ) func_type=EFG_NTICKS;
        if ( func_name=="expire_time" ) func_type=EFG_EXPIRE;
        if ( func_name=="expire" ) func_type=EFG_EXPIRE;
        if ( func_name=="remains_time" ) func_type=EFG_REMAINS;
        if ( func_name=="remains" ) func_type=EFG_REMAINS;
        if ( func_name=="cast_time" ) func_type=EFG_TCAST;
        if ( func_name=="execute_time" ) func_type=EFG_TCAST;
        if ( func_name=="cast" ) func_type=EFG_TCAST;
        // now create node if known global value
        if ( func_type>0 )
        {
          global_expression_t* g_func= new global_expression_t( func_type,e,act );
          root= g_func;
        }
        else
          if ( prefix!=EXPF_NONE )
            warn( 3,action,"Could not find value/function (%s) for %s", name.c_str(), e.c_str() );
      }
      // Global functions, player, target or sim related
      if ( ( ( prefix==EXPF_GLOBAL )||( prefix==EXPF_NONE ) ) && !root )
      {
        int glob_type=0;
        //first names that are not expressions, but option settings
        if ( name=="allow_early_recast" )
        {
          action->is_ifall=1;
          e="1";
        }
        // now regular names
        if ( name=="time" ) glob_type=EFG_TIME;
        if ( name=="time_to_die" ) glob_type=EFG_TTD;
        if ( name=="health_percentage" ) glob_type=EFG_HP;
        if ( name=="health_perc" ) glob_type=EFG_HP;
        if ( name=="vulnerable" ) glob_type=EFG_VULN;
        if ( name=="invulnerable" ) glob_type=EFG_INVUL;
        if ( name=="in_combat" ) glob_type=EFG_COMBAT;
        if ( name=="moving" ) glob_type=EFG_MOVING;
        if ( name=="move" ) glob_type=EFG_MOVING;
        if ( name=="distance" ) glob_type=EFG_DISTANCE;
        if ( name=="mana" ) glob_type=EFG_MANA;
        if ( name=="rage" ) glob_type=EFG_RAGE;
        if ( name=="energy" ) glob_type=EFG_ENERGY;
        if ( name=="focus" ) glob_type=EFG_FOCUS;
        if ( name=="runic" ) glob_type=EFG_RUNIC;
        if ( name=="resource" ) glob_type=EFG_RESOURCE;
        if ( name=="mana_perc" ) glob_type=EFG_MANA_PERC;
        if ( name=="mana_percentage" ) glob_type=EFG_MANA_PERC;
        if ( name=="rage_perc" ) glob_type=EFG_RAGE_PERC;
        if ( name=="rage_percentage" ) glob_type=EFG_RAGE_PERC;
        if ( name=="energy_perc" ) glob_type=EFG_ENERGY_PERC;
        if ( name=="energy_percentage" ) glob_type=EFG_ENERGY_PERC;
        if ( name=="focus_perc" ) glob_type=EFG_FOCUS_PERC;
        if ( name=="focus_percentage" ) glob_type=EFG_FOCUS_PERC;
        if ( name=="runic_perc" ) glob_type=EFG_RUNIC_PERC;
        if ( name=="runic_percentage" ) glob_type=EFG_RUNIC_PERC;
        if ( name=="resource_perc" ) glob_type=EFG_RESOURCE_PERC;
        if ( name=="resource_percentage" ) glob_type=EFG_RESOURCE_PERC;
        // now create node if known global value
        if ( glob_type>0 )
        {
          global_expression_t* g_func= new global_expression_t( glob_type,e,action );
          root= g_func;
        }
      }
      // Options(10), Glyphs(12), Items(13), Gear(14) - just existence, so "glyph.life_tap"
      if (  ( !root ) &&
            ( ( prefix==EXPF_OPTION )||( prefix==EXPF_GLYPH )||( prefix==EXPF_GEAR )||( prefix==EXPF_ITEM ) ) )
      {
        double item_value=0;
        bool item_found=false;
        if ( ( ( prefix==EXPF_GLYPH )||( prefix==EXPF_GEAR ) ) && !item_found )   //glyphs
        {
          std::vector<std::string> glyph_names;
          int num_glyphs = util_t::string_split( glyph_names, action->player->glyphs_str, ",/" );
          for ( int i=0; i < num_glyphs; i++ )
            if ( glyph_names[ i ] == name )
            {
              item_value=1;
              item_found=true;
            }
        };
        if ( ( ( prefix==EXPF_ITEM )||( prefix==EXPF_GEAR ) ) && !item_found )  //items
        {
          for ( int i=0; i < SLOT_MAX; i++ )
          {
            item_t& item = action->player->items[ i ];
            if ( item.active()&& ( ( item.encoded_name_str==name ) || ( strcmp( item.slot_name(),name.c_str() )==0 ) ) )
            {
              item_found=true;
              //if suffix present, return stat value, like "item.head.hit>35", but no need atm...
              int sfx_idx=0;
              if ( suffix!="" )
              {
                for ( int i=1; i<STAT_MAX; i++ )
                  if ( tolower( util_t::stat_type_abbrev( i ) )==suffix )
                    sfx_idx=i;
              }
              if ( sfx_idx )
                item_value=item.stats.get_stat( sfx_idx );
              else
                item_value=1;
            }
          }
        };
        if ( !item_found )
        { //other options
          option_t* opt_found=0;
          //first look in player options
          for ( unsigned int i=0; ( i<action->player->options.size() )&&!opt_found; i++ )
          {
            option_t& opt=action->player->options[i];
            if ( ( opt.name==name )&&( opt.address ) )   opt_found=&opt;
          }
          // if not found, check sim options
          if ( !opt_found )
            for ( unsigned int i=0; ( i<action->sim->options.size() )&&!opt_found; i++ )
            {
              option_t& opt=action->sim->options[i];
              if ( ( opt.name==name )&&( opt.address ) )   opt_found=&opt;
            }
          // now get value
          if ( opt_found )
          {
            item_found=true;
            switch ( opt_found->type )
            {
            case OPT_APPEND:
            case OPT_STRING: if ( ""!=*( ( std::string* ) opt_found->address ) )  item_value=1;      break;
            case OPT_BOOL:
            case OPT_INT:    item_value=*( ( int* ) opt_found->address ) ;                         break;
            case OPT_FLT:    item_value=*( ( double* ) opt_found->address ) ;                      break;
            }
          }
        };
        //create constant node with this. Value will be zero(false) if not found
        root= new act_expression_t( AEXP_VALUE,e,item_value );
      }
      // if nothing found so far, try function extensions
      if ( !root )
      {
        root= action->create_expression( name, pfx_candidate, suffix,expected_type );
      }
      // if still nothing, try aliases
      if ( !root )
      {
        std::string alias= action->sim->alias.find( e );
        if ( alias!="" )
        {
          std::string prot_name="*"+e;
          prot_name+="*";
          // check infinite loops due to recursion
          if ( alias_protect.find( prot_name )!=std::string::npos )
            warn( 3,action,"Alias (%s) using same alias in infinite loop !", e.c_str() );
          alias_protect+=prot_name;
          root= create( action, alias, alias_protect, expected_type );
        }
      }
    }

  }
  //return result for act_expression_t::create
  return root;
}

//return name of operand , based on type
std::string act_expression_t::op_name( int op_type )
{
  if ( op_type<=AEXP_NONE ) return "NONE";
  if ( op_type==AEXP_VALUE ) return "VALUE";
  if ( op_type==AEXP_INT_PTR ) return "INT_PTR";
  if ( op_type==AEXP_DOUBLE_PTR ) return "DOUBLE_PTR";
  if ( op_type==AEXP_BUFF ) return "BUFF";
  if ( op_type==AEXP_FUNC ) return "FUNC";
  // now check list
  for ( int i=0; ( AEXP_operators[i].type!=AEXP_NONE ); i++ )
    if ( AEXP_operators[i].type==op_type )
      return AEXP_operators[i].name;
  //if none found
  return "Unknown operator!";
}

// fill expression into string, for debug purpose
std::string act_expression_t::to_string( int level )
{
  std::string s="\n";
  std::string lvl_tab="";
  for ( int i=0; i<level; i++ ) lvl_tab+="  ";
  char buff[100];
  sprintf( buff,"%d",level );
  s+=lvl_tab+"["+buff+"]:  "+op_name( type );
  if ( type==AEXP_VALUE )
  {
    sprintf( buff,"%1.2lf",value );
    s=s+"("+buff+") ";
  }
  s+="  : "+exp_str+"\n";
  if ( operand_1 ) s+=operand_1->to_string( level+1 );
  if ( operand_2 ) s+=operand_2->to_string( level+1 );
  return s;
}

// **************************************************************************
//
// ALIAS_T  implementation
//
// **************************************************************************


void alias_t::add( std::string& name, std::string& value )
{
  //check validity
  if ( name=="" )
  {
    util_t::printf( "Alias definition must be in name=text  format\n" );
    assert( 0 );
  }
  // if already exists
  unsigned int i=0;
  for ( ; i<alias_name.size(); i++ )
    if ( name==alias_name[i] )
      break;
  // if found, return alias
  if ( i<alias_name.size() )
  {
    // rewrite old value
    alias_value[i]=value;
    util_t::printf( "Redefinition of alias (%s): %s",name.c_str(), value.c_str() );
  }
  else
  {
    // put new pair in list
    alias_name.push_back( name );
    alias_value.push_back( trim( value ) );
  }
}


void alias_t::init_parse()
{
  std::vector<std::string> parts;
  unsigned int num_parts = util_t::string_split( parts, alias_str, "/\\" );
  for ( unsigned int i=0; i<num_parts; i++ )
  {
    std::string alx=trim( parts[i] );
    std::string name="";
    if ( alx!="" )
    {
      size_t p= alx.find( "=" );
      if ( p!=std::string::npos )
      {
        name=tolower( alx.substr( 0,p ) );
        alx.erase( 0,p+1 );
      }
      // put pair in list
      add( name,alx );
    }
  }
}

std::string alias_t::find( std::string& candidate )
{
  // try to find that name
  std::string name=tolower( trim( candidate ) );
  unsigned int i=0;
  for ( ; i<alias_name.size(); i++ )
    if ( name==alias_name[i] )
      break;
  // if found, return alias
  if ( i<alias_name.size() )
    return alias_value[i];
  else
    return "";
}

// convert full descriptive name into "option name", without rewriting input
// spaces become underscores, all lower letters, remove apostrophes and colons
std::string proper_option_name( const std::string& full_name )
{
  std::string new_name=full_name;
  armory_t::format( new_name );
  return new_name;
}

