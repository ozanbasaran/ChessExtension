\echo Use "CREATE EXTENSION chess" to load this file. \quit

/***CHESSBOARD***/

/******************************************************************************
 * Input/Output
 ******************************************************************************/

CREATE OR REPLACE FUNCTION chessboard_in(cstring)
  RETURNS chessboard
  AS 'MODULE_PATHNAME'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION chessboard_out(chessboard)
  RETURNS cstring
  AS 'MODULE_PATHNAME'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE TYPE chessboard (
  internallength = 100, -- to be adapted
  input          = chessboard_in,
  output         = chessboard_out
);

/******************************************************************************
 * Constructor
 ******************************************************************************/

CREATE FUNCTION chessboard(varchar(72), char, varchar(5), varchar(3), int, int)
  RETURNS chessboard
  AS 'MODULE_PATHNAME', 'chessboard_constructor'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

/***CHESSGAME***/

/******************************************************************************
 * Input/Output
 ******************************************************************************/

CREATE OR REPLACE FUNCTION chessgame_in(cstring)
  RETURNS chessgame
  AS 'MODULE_PATHNAME'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION chessgame_out(chessgame)
  RETURNS cstring
  AS 'MODULE_PATHNAME'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE TYPE chessgame (
  internallength = 2000, -- to be adapted
  input          = chessgame_in,
  output         = chessgame_out
);

/******************************************************************************
 * Constructor
 ******************************************************************************/

CREATE FUNCTION chessgame(varchar(1406))
  RETURNS chessgame
  AS 'MODULE_PATHNAME', 'chessgame_constructor'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
  
 /*****************************************************************************
 * Accessing values
 *****************************************************************************/

CREATE FUNCTION getBoard(chessgame, int)
  RETURNS chessboard
  AS 'MODULE_PATHNAME', 'getBoard'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;


CREATE FUNCTION hasBoard(chessgame, chessboard, int)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'hasBoard'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE FUNCTION hasOpening(chessgame, chessgame)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'hasOpening'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;


CREATE FUNCTION getOpening(chessgame, int)
  RETURNS chessgame
  AS 'MODULE_PATHNAME', 'getOpening'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;



/* B-Tree comparison functions */
CREATE OR REPLACE FUNCTION chessgame_lt(chessgame, chessgame)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'chessgame_lt'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE OR REPLACE FUNCTION chessgame_lte(chessgame, chessgame)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'chessgame_lte'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION chessgame_eq(chessgame, chessgame)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'chessgame_eq'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION chessgame_gt(chessgame, chessgame)
  RETURNS boolean
  AS 'MODULE_PATHNAME','chessgame_gt'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE OR REPLACE FUNCTION chessgame_gte(chessgame, chessgame)
  RETURNS boolean
  AS 'MODULE_PATHNAME','chessgame_gte'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;



 /* B-Tree comparison operators */

CREATE OPERATOR = (
  LEFTARG = chessgame, RIGHTARG = chessgame,
  PROCEDURE = chessgame_eq,
  COMMUTATOR = =, NEGATOR = <>
);
CREATE OPERATOR < (
  LEFTARG = chessgame, RIGHTARG = chessgame,
  PROCEDURE = chessgame_lt,
  COMMUTATOR = >, NEGATOR = >=
);
CREATE OPERATOR <= (
  LEFTARG = chessgame, RIGHTARG = chessgame,
  PROCEDURE = chessgame_lte,
  COMMUTATOR = >=, NEGATOR = >
);
CREATE OPERATOR >= (
  LEFTARG = chessgame, RIGHTARG = chessgame,
  PROCEDURE = chessgame_gte,
  COMMUTATOR = <=, NEGATOR = <
);
CREATE OPERATOR > (
  LEFTARG = chessgame, RIGHTARG = chessgame,
  PROCEDURE = chessgame_gt,
  COMMUTATOR = <, NEGATOR = <=
);


/* B-Tree support function */

CREATE OR REPLACE FUNCTION chessgame_cmp(chessgame, chessgame)
  RETURNS integer
  AS 'MODULE_PATHNAME', 'chessgame_cmp'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

/******************************************************************************/

/* B-Tree operator class */

CREATE OPERATOR CLASS chessgame_ops
DEFAULT FOR TYPE chessgame USING btree
AS
        OPERATOR        1       <  ,
        OPERATOR        2       <= ,
        OPERATOR        3       =  ,
        OPERATOR        4       >= ,
        OPERATOR        5       >  ,
        FUNCTION        1       chessgame_cmp(chessgame, chessgame);

/******************************************************************************/