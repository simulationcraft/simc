#ifndef CPP_SEMVER_PEG_HPP
#define CPP_SEMVER_PEG_HPP

#include "../base/type.hpp"

#include <tao/pegtl.hpp>

#include <memory>
#include <cassert>

namespace peg
{
  using namespace semver;

  // ------------------ Defining grammar with PEGTL ------------------------------ //

  namespace p = tao::TAO_PEGTL_NAMESPACE;

  // XXX: extra rules for less strict whitespace matching
  struct some_space : p::plus< p::space > {};
  struct any_space  : p::star< p::space > {};

  // '0' | ['1'-'9'] ( ['0'-'9'] ) *
  struct nr : p::sor< p::one<'0'>,
                      p::seq< p::ranges<'1', '9'>, p::star< p::ranges<'0', '9'> > >
                    > {};

  // 'x' | 'X' | '*' | nr
  struct xr : p::sor< p::one<'x', 'X', '*'>, nr > {};

  // nr | [-0-9A-Za-z]+
  struct part : p::sor< nr,
                        p::plus< p::ranges<'a', 'z', 'A', 'Z', '0', '9', '-'> >
                      > {};

  // part ( '.' part ) *
  struct parts : p::seq< part, p::star< p::one<'.'>, part > > {};

  // parts
  struct build : parts {};

  // parts
  struct pre : parts {};

  // ( '-' pre )? ( '+' build )?
  struct qualifier : p::seq< p::opt< p::seq< p::one<'-'>, pre > >,
                             p::opt< p::seq< p::one<'+'>, build > >
                           > {};

  // XXX: extra rules to capture each 'xr' from 'partial'
  struct partial_major : xr {};
  struct partial_minor : xr {};
  struct partial_patch : xr {};

  // xr ( '.' xr ( '.' xr qualifier ? )? )?
  // XXX: accepting leading optional 'v'
  struct partial : p::seq< p::opt< p::one<'v','V'> >,
                           partial_major,
                           p::opt< p::one<'.'>,
                                   partial_minor,
                                   p::opt< p::one<'.'>,
                                           partial_patch,
                                           p::opt< qualifier >
                                         >
                                  >
                          > {};

  // '~' partial
  struct tilde : p::seq< p::one<'~'>, partial > {};

  // '^' partial
  struct caret : p::seq< p::one<'^'>, partial > {};

  // XXX: extra rules to capture comparator
  struct primitive_op_lte : p::string<'<','='> {};
  struct primitive_op_gte : p::string<'>','='> {};
  struct primitive_op_gt  : p::string<'>'> {};
  struct primitive_op_lt  : p::string<'<'> {};
  struct primitive_op_eq  : p::string<'='> {};

  // ( '<' | '>' | '>=' | '<=' | '=' | ) partial
  // XXX: rule order matters!
  struct primitive : p::seq< p::sor< primitive_op_lte,
                                     primitive_op_gte,
                                     primitive_op_gt,
                                     primitive_op_lt,
                                     primitive_op_eq,
                                     p::success
                                   >,
                             partial
                           > {};

  // primitive | partial | tilde | caret
  struct simple : p::sor< primitive, partial, tilde, caret > {};

  // partial ' - ' partial
  struct hyphen : p::seq< partial,
                          some_space,
                          p::one<'-'>,
                          some_space,
                          partial
                        > {};

  // hyphen | simple ( ' ' simple ) * | ''
  struct range : p::sor< hyphen,
                         p::seq< simple,
                                 p::star< some_space, simple >
                               >,
                         any_space
                       > {};

  // ( ' ' ) * '||' ( ' ' ) *
  struct logical_or : p::seq< any_space,
                              p::string<'|','|'>,
                              any_space
                            > {};

  // range ( logical-or range ) *
  struct range_set : p::must< p::seq< any_space,
                                      range,
                                      p::star< logical_or, range >,
                                      any_space,
                                      p::eof
                                    >
                            > {};

  // ------------------ Parser actions for PEGTL ------------------------------ //

  namespace internal
  {
    /// internal stack buffer used during parsing
    struct stacks
    {
      std::unique_ptr< syntax::simple >   partial;
      std::vector< syntax::simple >       stack_partial;
      std::vector< syntax::simple >       stack_primitive;
      std::vector< syntax::simple >       stack_simple;
      std::vector< syntax::range  >       stack_range;
    };

  }

  template< typename Rule >
  struct action : p::nothing< Rule >
  {
  };

  template<>
  struct action< pre >
  {
    template< typename Input >
    static void apply(const Input& in, syntax::range_set&, internal::stacks& stacks)
    {
      assert(stacks.partial != nullptr);

      stacks.partial->pre = in.string();
    }
  };

  template<>
  struct action< build >
  {
    template< typename Input >
    static void apply(const Input& in, syntax::range_set&, internal::stacks& stacks)
    {
      assert(stacks.partial != nullptr);

      stacks.partial->build = in.string();
    }
  };

  template<>
  struct action< partial >
  {
    template< typename Input >
    static void apply(const Input&, syntax::range_set&, internal::stacks& stacks)
    {
      assert(stacks.partial != nullptr);

      // reduce to 1 'partial' onto stack
      stacks.stack_partial.emplace_back(std::move(*stacks.partial));
      stacks.partial.reset();
    }
  };

  template<>
  struct action< partial_major >
  {
    template< typename Input >
    static void apply(const Input& in, syntax::range_set&, internal::stacks& stacks)
    {
      if (!stacks.partial)
        stacks.partial = std::unique_ptr< syntax::simple >(new syntax::simple());

      try
      { //a parse a number
        stacks.partial->major = syntax::xnumber(std::stoi(in.string()));
      }
      catch (std::invalid_argument&)
      { // or as a wildcard
        stacks.partial->major = { };
      }
    }
  };

  template<>
  struct action< partial_minor >
  {
    template< typename Input >
    static void apply(const Input& in, syntax::range_set&, internal::stacks& stacks)
    {
      assert(stacks.partial != nullptr);

      try
      { // parse a number
        stacks.partial->minor = syntax::xnumber(std::stoi(in.string()));
      }
      catch (std::invalid_argument&)
      { // or as a wildcard
        stacks.partial->minor = { };
      }
    }
  };

  template<>
  struct action< partial_patch >
  {
    template< typename Input >
    static void apply(const Input& in, syntax::range_set&, internal::stacks& stacks)
    {
      assert(stacks.partial != nullptr);

      try
      { // parse a number
        stacks.partial->patch = syntax::xnumber(std::stoi(in.string()));
      }
      catch (std::invalid_argument&)
      { // or as a wildcard
        stacks.partial->patch = { };
      }
    }
  };

  template<>
  struct action< tilde >
  {
    template< typename Input >
    static void apply(const Input&, syntax::range_set&, internal::stacks& stacks)
    {
      assert(!stacks.stack_partial.empty());

      // add 'tilde' to the last 'partial'
      stacks.stack_partial.back().cmp = syntax::comparator::tilde;
    }
  };

  template<>
  struct action< caret >
  {
    template< typename Input >
    static void apply(const Input&, syntax::range_set&, internal::stacks& stacks)
    {
      assert(!stacks.stack_partial.empty());

      // add 'caret' to the last 'partial'
      stacks.stack_partial.back().cmp = syntax::comparator::caret;
    }
  };

  template<>
  struct action< primitive >
  {
    template< typename Input >
    static void apply(const Input&, syntax::range_set&, internal::stacks& stacks)
    {
      assert(!stacks.stack_partial.empty());

      // reduce all 'partial's to 1 'primitive'
      stacks.stack_primitive.emplace_back(std::move(stacks.stack_partial.back()));
      stacks.stack_partial.clear();
    }
  };

  template<>
  struct action< primitive_op_lt >
  {
    template< typename Input >
    static void apply(const Input&, syntax::range_set&, internal::stacks& stacks)
    {
      assert(!stacks.partial);

      stacks.partial = std::unique_ptr< syntax::simple >(new syntax::simple());
      stacks.partial->cmp = syntax::comparator::lt;
    }
  };

  template<>
  struct action< primitive_op_gt >
  {
    template< typename Input >
    static void apply(const Input&, syntax::range_set&, internal::stacks& stacks)
    {
      assert(!stacks.partial);

      stacks.partial = std::unique_ptr< syntax::simple >(new syntax::simple());
      stacks.partial->cmp = syntax::comparator::gt;
    }
  };

  template<>
  struct action< primitive_op_gte >
  {
    template< typename Input >
    static void apply(const Input&, syntax::range_set&, internal::stacks& stacks)
    {
      assert(!stacks.partial);

      stacks.partial = std::unique_ptr< syntax::simple >(new syntax::simple());
      stacks.partial->cmp = syntax::comparator::gte;
    }
  };

  template<>
  struct action< primitive_op_lte >
  {
    template< typename Input >
    static void apply(const Input&, syntax::range_set&, internal::stacks& stacks)
    {
      assert(!stacks.partial);

      stacks.partial = std::unique_ptr< syntax::simple >(new syntax::simple());
      stacks.partial->cmp = syntax::comparator::lte;
    }
  };

  template<>
  struct action< primitive_op_eq >
  {
    template< typename Input >
    static void apply(const Input&, syntax::range_set&, internal::stacks& stacks)
    {
      assert(!stacks.partial);

      stacks.partial = std::unique_ptr< syntax::simple >(new syntax::simple());
      stacks.partial->cmp = syntax::comparator::eq;
    }
  };

  template<>
  struct action< simple >
  {
    template< typename Input >
    static void apply(const Input&, syntax::range_set&, internal::stacks& stacks)
    {
      if (!stacks.stack_primitive.empty())
      {
        // reduce 1 'primitive' to 1 'simple'
        stacks.stack_simple.emplace_back(std::move(stacks.stack_primitive.back()));
        stacks.stack_primitive.pop_back();
      }
      else if (!stacks.stack_partial.empty())
      {
        // reduce all 'partial's to 1 'simple'
        stacks.stack_simple.emplace_back(std::move(stacks.stack_partial.back()));
        stacks.stack_partial.clear();
      }
      else
      {
        assert(!"primitive or partial unavailable");
      }
    }
  };

  template<>
  struct action< hyphen >
  {
    template< typename Input >
    static void apply(const Input&, syntax::range_set&, internal::stacks& stacks)
    {
      assert(stacks.stack_partial.size() >= 2);

      // reduce 2 'partial's to 1 'hyphon'
      syntax::simple to = std::move(stacks.stack_partial.back());
      stacks.stack_partial.pop_back();
      syntax::simple from = std::move(stacks.stack_partial.back());
      stacks.stack_partial.pop_back();

      // translate hyphon to a range set
      from.cmp = syntax::comparator::gte;
      to.cmp = syntax::comparator::lte;

      syntax::range hyphon;
      hyphon.as_hyphon = true;
      hyphon.and_set.emplace_back(std::move(from));
      hyphon.and_set.emplace_back(std::move(to));

      stacks.stack_range.emplace_back(std::move(hyphon));
    }
  };

  template<>
  struct action< range >
  {
    template< typename Input >
    static void apply(const Input& in, syntax::range_set&, internal::stacks& stacks)
    {
      if (!stacks.stack_simple.empty())
      {
        // reduce all 'simple's to 1 'range'
        syntax::range simple_set;
        simple_set.and_set.swap(stacks.stack_simple);
        // imply stacks.stack_simple.clear();
        stacks.stack_range.emplace_back(std::move(simple_set));
      }
      else if (in.string().find_first_not_of(" \t\r\n") == std::string::npos)
      {
        // the input may be a blank string which is allowed as an implcit *.*.* range
        syntax::range implicit_set;
        implicit_set.and_set.emplace_back(syntax::simple());
        stacks.stack_range.emplace_back(std::move(implicit_set));
      }
    }
  };

  template<>
  struct action< range_set >
  {
    template< typename Input >
    static void apply(const Input&, syntax::range_set& result, internal::stacks& stacks)
    {
      // assign result
      result.or_set.swap(stacks.stack_range);

      // stack shall be cleared after a successful parse
      assert(!stacks.partial &&
        stacks.stack_partial.empty() &&
        stacks.stack_primitive.empty() &&
        stacks.stack_simple.empty() &&
        stacks.stack_range.empty());
    }
  };

  // ------------------ Trigger PEGTL parser function ------------------------------ //

  inline syntax::range_set parser(const std::string input)
  {
    namespace pegtl = tao::TAO_PEGTL_NAMESPACE;
    try
    {
      syntax::range_set result;
      internal::stacks stacks;
      pegtl::string_input<> in(input, "input string");
      pegtl::parse< range_set, action >(in, result, stacks);
      return result;
    }
    catch (pegtl::parse_error const&)
    {
      throw semver_error("parsing error due to invalid syntax");
    }
  }
}

#endif