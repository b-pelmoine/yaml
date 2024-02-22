#pragma once

#include <filesystem>
#include <fstream>
#include <ios>
#include <iterator>
#include <optional>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

namespace yaml {

template<typename T>
concept schematic = requires(T _schema, std::string_view _content) { _schema.load(_content); };

struct node
{
  virtual ~node() = default;
  template<typename Type> decltype(auto) as(this auto &&_self) { return std::get<Type>(_self.data); }
  template<typename Type> decltype(auto) try_as(this auto &&_self) { return std::get_if<Type>(_self.data); }
  template<typename Type> decltype(auto) is(this auto &&_self) { return std::holds_alternative<Type>(_self.data); }
};

using node_ref = std::shared_ptr<node>;

struct failsafe
{
  using scalar = std::string;
  using sequence = std::vector<node_ref>;
  using mapping = std::vector<std::pair<node_ref, node_ref>>;
  using node_data = std::variant<scalar, sequence, mapping>;

  struct node : yaml::node
  {
    node() = default;
    node(node_data &&_data) : data(_data) {}
    node_data data;
  };

  decltype(auto) get_node(this auto &&_self, node_ref _node) { return *static_cast<node *>(_node.get()); }

  node load(std::string_view)
  {
    return node{ mapping{ std::make_pair(std::make_shared<node>(scalar{ "values" }),
      std::make_shared<node>(sequence{ std::make_shared<node>(scalar{ "one" }),
        std::make_shared<node>(scalar{ "two" }),
        std::make_shared<node>(scalar{ "three" }) })) } };
  }
};

template<schematic Schema = failsafe> auto load(std::string_view _content) { return Schema{}.load(_content); }

}// namespace yaml

// ---

// document: part of a yaml stream
// a stream can be composed of multiple documents
// each document is separated by a marker
// --- starts a document
// ... ends a document
// identation is defined by spaces
// each node must be indented further than its parent node,
// all sibling nodes must use the same indentation level

// node: scalar, sequence, mapping [kv]
// each node has a tag, it can be specified or deduced
// a node can be referenced multiple times
// the first occurence is the anchor and each subsequent occurence is serialized as an alias node
// redefining an anchor overrides it

// ...
// ---

// stream validation has several failure points ~

// when parsing: ill formed of unindentified (no representation)
// each alias shall refer to a previous node identifier by an anchor
// recovery is allowed if a mechanism for reporting error as been provided

// when composing:

// - document is unresolved (partial representation)
// each node must be resolved to a unique tag (either local or global)
// resolving a tag depends on:
//  1/ the non-specific tag of the node
//  2/ the path leading from the root to the node
//  3/ the content (aka kind) of the node
// resolution ignores comments, identation and node style
// ! tagged nodes should be resolved as tag:yaml.org,2002:seq, tag:yaml.org,2002:map or tag:yaml.org,2002:str

// - document is unrecognized of invalid (partial representation if scalar otherwise complete representation)
// a node is valid if it has a recognized tag and its content satisfies the contraints imposed by this tag
// even given a complete representation a native data structure cannot be constructed from it

// when constructing: if a native representation is not available

// ...
// ---

// BOM: may appear at the start of any document
// x00 x00 xFE xFF | x00 x00 x00 any > UTF-32 BE
// xFF xFE x00 x00 | any x00 x00 x00 > UTF-32 LE
// xFE xFF | x00 any > UTF-16 BE
// xFF xFE | any x00 > UTF-16 LE
// xFE xFF | x00 any > UTF-16 BE
// xEF xBB xBF | any > UTF-8

// indicator charactors

// block struct indicator

// a block sequence starts with a hyphen x2D -
// a mapping key is denoted by a question mark x3F ?
// a mapping value is denoted by a colon x3A :

// flow collection indicator

// a flow entry is ended by a comma x2C ,
// a flow sequence starts with a left bracket x5B [
// a flow sequence end with a right bracket x5D ]
// a flow mapping starts with a left brace x7B {
// a flow mapping end with a right brace x7D }

// node property indicator

// a node anchor is denoted by an ampersand x26 &
// a node alias is denoted by an asterisk x2A *
// a node tag is denoted by the exclamation x21 !

// block scalar indicator

// a literal block scalar is denoted by a pipe x7C |
// a folded block scalar is denoted by a gretaer than x3E >

// whitespaces & linebreaks

// a linebreak is either a carriage return x0A or a line feed x0A
// a whitespace is either a space x20 or a tab x09

// misc

// a comment is denoted by a sharp x23 #
// a string is denoted by matching single or double quotes x27 ' x22 "
// a directive is denoted by a percent x25 %

// at x40 @ and grave accent x60 ` are reserved for future use and cannot be used to start a scalar

// processors should end the document with an explicit line break

// ...
// ---

// structural production

// structure is determined by indentation. Indentation is defined as a zero or more space characters at the start of a
// line a block style construct is terminated when encountering a line which is less indented than the construct
// - ? and : used to denote block collection may be used as indentation, this is handled case by case

// white space characters are used as separation between tokens within a line
// separation spaces are a presentation detail and should not convey any content

// empty lines are non-content prefix followed by a line break
// if a line break is followed by an empty line it is trimmed otherwise it is converted to single space x20

// flow indicators depend on explicit indicators rather than indentation to convey structure
// spaces preceding or following the text in a line are presentation detail, once discarded all line breaks are folded

// a line may end with a comment and it may be followed by additional comment lines

// ...
// ---

// directives

// a directive is composed of the % token followed by a name and parameters separated by white spaces
// the only defined directives are YAML and TAG, any other directive defined should emit a warning

// YAML {version} - specifies the version the document conform to ex: "%YAML 1.2"

// if unspecified the document is assumed to conform to the 1.2 version
// a higher minor version should emit a warning, a higher major version should be rejected and emit an error
// a lower minor version should emit a warning on points of incompatibility
// defining the directive twice should emit an error

// TAG {tag handle} {tag prefix} - establishes a tag shorthand notation for specifying node tags ex: "%TAG !yaml!
// tag:yaml.org,2002:"

// defining the directive with the same handle twice should emit an error
// There are 3 tag handle variants: primary, secondary and named tag handles
// primary is composed of a single ! character, shorthand using this handle are interpreted as local tags
// secondary is composed of a double ! character (!!), by default the prefix associated with this handle is
// "tag:yaml.org,2002:" named handle is a presentation detail and must not convey content information, the handle can be
// dropped by the processor after parsing There are 2 prefix variants: local and global local prefix begins with a !
// character ex: "%TAG !m! !my-" shorthands using the handle are expanded to a local tag. 2 documents may assign
// different semantics to the same local tag global prefix begins with a character other than ! ex: "%TAG !e!
// tag:example.com,2000:app/" shorthands using the handle are expanded to a global unique uri. Every document must share
// the same semantics to the same global tag

// ...
// ---

// node properties

// each node may have 2 optional properties: anchor and tag, in addition to its content. these properties may be
// specified in any order (before the content)

// node tag

// a tag may be written verbatim by surrounding it with the < and > characters. verbatim tags are not subject to tag
// resolution

// a verbatim tag must either begin with a ! (local tag) or be a valid URI (global tag) ex: "!<tag:yaml.org,2002:str>
// foo : !<!bar> baz" a tag shorthand consists of a valid tag handle followed by a non empty suffix a suffix must not
// contain any of the following characters: "![]{}," to be included these characters should be escaped using the %
// character valid: %TAG !e! tag:example.com,2000:app/ --- - !local foo - !!str bar - !e!tag%21 baz ... invalid: %TAG
// !e! tag:example.com,2000:app/ --- - !e! foo - !h!bar baz ... !e! has no suffix and !h! wasn't declared

// a non-specific tag is assigned to nodes without a tag property. ! is used for plain scalar and ? for all other nodes
// using ! alone as a non specific tag forces the node to be interpreted as seq, map or str according to its kind ex: [
// 12, ! 12 ] -> [ 12, "12" ]

// node anchor

// an anchor is denoted by the & indicator, it marks the node for future reference.
// an anchored node does not need to be references by any alias node
// it is valid for all nodes to be anchored
// anchor names must not contain any of the following characters: "[]{},"

// ...
// ---

// flow style productions

// alias nodes

// an alias is denoted by the * indicator, it refers to the most recent preceding node having the same anchor
// using an alias node without a preceding anchor emits an error

// empty nodes

// an empty node is a node with empty content, they are equivalent to a plain scalar with an empty value or null value
// both the node content and the node property are optional

// flow scalar styles (presentation detail with exception of plain scalars used in tag resolution)

// true for all:
//  restricted to a single line when contained inside an implicit key
//  all leading an trailing white space characters on each line are excluded from the content
//  empty line are consumed as part of the line folding

// double quoted
// the only style capable of expressing arbitrary strings by using escape sequences (with the added cost of having to
// escape the \ and " characters) line breaks are subject to flow line folding, which discards any trialing white space
// characters a line break can be escaped to preserve preceding white space characters each continuation line must
// contain at least one non space chacters

// single quoted
// the only form of escaping is the doubling of the single quote character meaning \ and " may be used freely
// single quoted scalars are therefore limited to printable characters

// plain
// does not allow any form of escaping, making it the most limited and context sensitive but also the most readable
// a plain scalar must not be empty or contain leading or trailing white space characters
// plain scalars must not begin with most indicators however : ? and - may be used as the first character if followed by
// a non-space safe character plain scalars must not contain the ": " and " #" character combinations as it would cause
// ambiguity with mapping k/v pairs and comments inside flow collections or, when used as implicit keys, they must not
// contain the [ ] { } and , characters due to ambiguity with flow collection

// flow collection styles

// flow collections may be nested within a block collection, another flow collection or be a part of an implicit key or
// block key entries are terminated by the "," indicator, the final one lay be omitted

// flow sequences
// denoted by surrounding [ ] characters ex: [ one, two ]
// any flow node may be used as a flow sequence entry [ [nested], [sequences] ]
// compact notation can be used for the case where a flow entry is a mapping with a single k/v pair ex: [ single: pair,
// another: one ]

// flow mappings
// denoted by surrounding { } characters ex: { one: two, three: four }
// if the ? mapping is specified the rest of the entry may be completely empty { ? one: two, three: four, ? }
// the value may be completely empty it's existence is already suggested by the : indicator ex: { :two, three: }
// the mapping value indicator must be separated from the value with a white space to not be confused with plain scalars
// "a:1" is a plain scalar and not a k/v pair exception to the rule: for JSON compatibility a key inside a flow mapping
// is JSON-like but a : is preferred on output ex: { "one":two, "three": four, "five": } == { "one": "two", "three":
// "four", "five": null } single pair flow mapping: if the ? indicator is specified
//  the syntax is identical to the general case
// if the ? indicator is omitted
//  to limit the amount of lookahead the : indicator must appear within at most 1024 unicode characters beyodn the start
//  of the key the key is restricted to a single line

// flow nodes
// JSON-like flow styles excluding alias nodes and plain scalars which are yaml content

// flow content
// - [ a, b ] -> [ [ "a", "b" ],
// - { c: d }      { "c": "d" },
// - "e"           "e",
// - f             "f",
// - &anchor g     "g",
// - *anchor       "g",
// - !!str         "" ]

// ...
// ---

// block style productions

// block scalar types (literal, folded) | or >

// block scalar headers
// a header with an optional comment followed by a non content line break itself followed by the content itself
// this is the only case where a comment must not be followed by additional comment lines

// indentation
//  the indentation indicator (1-9, number of leading spaces on each line up to the content identation level)
// if a block has an indentation indicator the content indentation level is equal to the indentation level of the block
// + the integer value if no indentation is given the indentation level is equal to the number of leadign spaces on the
// first non empty line of the content. if there is no non empty line the indentation is equal to the number of spaces
// on the longest line it is an error an any non empty line does not begin with a number of spaces greater or equal to
// the content indentation level it is an error for any leading empty lines to contain more spaces than the first non
// empty line

// chomping
// strip is specified by the - indicator, the final line break and any trailing empty lines are excluded from the
// content clip is the default behavior, the final line break is preserved bu any traling empty lines are excluded from
// the content keep is specified by the + indicator, the final line break and any trailing empty lines are preserved
// (line folding does not apply) explicit comment lines may follow the trailing empty lines and are treated as such as
// long as the first comment is less indented than the block scalar content an empty block scalar is considered as
// trailing lines and hence affected by chomping

// inside literal scalars (denoted by the | indicator) all characters including white spaces are considered content

// inside folded scalars (denoted by the > indicator) the same rules as literal scalars applies with the addition of
// line folding
//  folding allows long lines to be broken anywhere a single space character separates 2 non-space characters
//  lines starting with white space characters (more indented lines) are not folded
//  line breaks and empty lines separating folded and more indented lines are not folded
//  the final line break and trailing empty lines are never folded

// block collection styles

// block sequences
// a series of nodes each denoted by a leading - indicator
// the indicator must be separated from the content by a white space meaning - can be used in a plain scalar if not
// followed by a white space ex: -42 the entry node may be either completely empty, a nested block node or use compact
// in line notation this type of collection cannot have any node properties

// block mappings
// a series of entries each presenting a k/v pair
// if the ? indicator is specified the optional value node must be specified on a separate line denoted by the :
// indicator if the ? indicator is omitted, similar to single k/v pair in flow mapping the line is limited to no more
// than 1024 unicode characters the : indicator must never be adjacent to the value there is no compact notation for in
// line values and while botht he key and valeu are optional the : is mandatory to prevent ambiguity with mutli line
// plain scalars compact in line notation may be nested inside block sequences and explict block mapping entries with
// the restriction that they cannot have any node properties

// block nodes
// flow nodes embedded inside block collections
// flow nodes must be indented by at least one more space than the parent block collection and may begin on a following
// line it's a this point that parsing needs to distinguish between a plain scalar and an implicit key starting a nested
// block mapping the node properties may span across several lines as long as they are indented by at least one more
// space than the block collection nested blocks may be indented by one less space to compensate - being perceived as
// indentation by most people expect if nested inside another block sequence

// ...
// ---

// document stream productions

// documents

// document markers
// directives may be ambiguous as it is valid to have a % character at the start of the line for example as part of a
// plain scalar at the start of the line to distinguish the 2 we use marker lines a % on the first line are assumed to
// be directives at the start of the document (the --- marker) the parser stops scanning for directives at the end of
// the document (the ... marker) the parser starts scanning for directives again % are again assumed to be directives on
// the line following the marker this marker does not necessarily indicate the existence of an actual following document

// bare documents
// does not begin with any directives or marker lines, only content
// the first non comment line may not start with a % character

// explicit documents
// start with a --- but no directives
// may be completely empty past this marker

// directives documents
// starts with some directives followed by an explicit ---

// streams
// consist of 0 or more documents
// if a document is not terminated by a ... marker then the following document must begin with a ---

// ...
// ---

// recommended schemas

// failsafe

// URI tag:yaml.org,2002:map - kind: Mapping
// represents an associative container where each key is unique and mapped to excatly one value
// the key type is unrestricted
// !!map { Clark: Evans, Ingy: döt Net, Oren: Ben-Kiki }

// URI tag:yaml.org,2002:seq - kind: Sequence
// represents collection indexed by sequential integers starting at 0
// !!seq [ Clark Evans, Ingy döt Net, Oren Ben-Kiki ]

// URI tag:yaml.org,2002:str - kind: Scalar
// represents a unicode string, bound to a native string like value
// canonical form: ""
// !!str "String: just a theory."

// all nodes with the ! non specific tag are resolved to one of the listed option according to their kind

// json

// URI tag:yaml.org,2002:null - kind: Scalar
// represents a lack of value, bound to a native null like value
// canonical form: null
// key with null value: !!null null [or] !!null null: value for null key

// URI tag:yaml.org,2002:bool - kind: Scalar
// represents a true/false value, bound to a native boolean type
// canonical form: either true or false
// !!bool false

// URI tag:yaml.org,2002:int - kind: Scalar
// represents a arbitrary sized finite mathematical integers, bound to a native integer type if possible
// an overflow of the value may emit an error, truncate it with a warning or find other manner to round trip it
// canonical form: decimal integer notation with a leading - for negative values matching the regular expression 0 | -?
// [1-9] [0-9]*
// !!int -12 [or] !!int 0 [or] !!int 34

// URI tag:yaml.org,2002:float - kind: Scalar
// represents an approximation to real numbers, including three special values (positive and negative infinity and “not
// a number”), bound to a native floating point value if possible canonical form: Either 0, .inf, -.inf, .nan or
// scientific notation matching the regular expression -? [1-9] ( \. [0-9]* [1-9] )? ( e [-+] [1-9] [0-9]* )?
// !!float -1 [or] !!float 0 [or] !!float 2.3e4 [or] !!float .inf [or] !!float .nan

// all nodes with the ! non specific tag are resolved to “tag:yaml.org,2002:seq”, “tag:yaml.org,2002:map” or
// “tag:yaml.org,2002:str” according to their kind collections with the ? non specific tag are resolved to
// “tag:yaml.org,2002:seq” or “tag:yaml.org,2002:map” according to their kind scalars with the ? non specific tag are
// resolved matched with a list of regular expressions, first match wins a on matching scalar should be considered an
// error

// Regular expression	                                            Resolved to tag

// null	                                                          tag:yaml.org,2002:null
// true | false	                                                  tag:yaml.org,2002:bool
// -? ( 0 | [1-9] [0-9]* )	                                      tag:yaml.org,2002:int
// -? ( 0 | [1-9] [0-9]* ) ( \. [0-9]* )? ( [eE] [-+]? [0-9]+ )?	tag:yaml.org,2002:float
// *	                                                            Error

// core

// uses the same tags as the JSON schema
// core is an extension of the JSON schema tag resolution
// a non mtaching scalar is resolved as a string instead of emiting an error

// Regular expression	                                                Resolved to tag

// null | Null | NULL | ~	                                            tag:yaml.org,2002:null
// /* Empty */	                                                      tag:yaml.org,2002:null
// true | True | TRUE | false | False | FALSE	                        tag:yaml.org,2002:bool
// [-+]? [0-9]+	                                                      tag:yaml.org,2002:int (Base 10)
// 0o [0-7]+	                                                        tag:yaml.org,2002:int (Base 8)
// 0x [0-9a-fA-F]+	                                                  tag:yaml.org,2002:int (Base 16)
// [-+]? ( \. [0-9]+ | [0-9]+ ( \. [0-9]* )? ) ( [eE] [-+]? [0-9]+ )?	tag:yaml.org,2002:float (Number)
// [-+]? ( \.inf | \.Inf | \.INF )	                                  tag:yaml.org,2002:float (Infinity)
// \.nan | \.NaN | \.NAN	                                            tag:yaml.org,2002:float (Not a number)
// *	                                                                tag:yaml.org,2002:str (Default)

// Other schemas

// additionnal local tags can be added that map directly to the native data structures
// it is strongly advised for these schemas to be based on the core schema for better interoperability

// ...