CREATE TABLE IF NOT EXISTS lineitem (
  l_orderkey    INT,
  l_partkey     INT,
  l_suppkey     INT,
  l_linenumber  INT,
  l_quantity    DECIMAL(15,2),
  l_extendedprice  DECIMAL(15,2),
  l_discount    DECIMAL(15,2),
  l_tax         DECIMAL(15,2),
  l_returnflag  CHAR(1),
  l_linestatus  CHAR(1),
  l_shipdate    DATE,
  l_commitdate  DATE,
  l_receiptdate DATE,
  l_shipinstruct CHAR(25),
  l_shipmode    CHAR(10),
  l_comment     VARCHAR(44)) USING columnar;
-- SELECT alter_columnar_table_set('lineitem', compression => 'pglz', stripe_row_limit => 300000, chunk_group_row_limit => 150000);
SELECT alter_columnar_table_set('lineitem', compression => 'pglz', stripe_row_limit => 150000, chunk_group_row_limit => 10000);
\timing
-- MODIFY the path. Example) /path_to/intel-iaa-eval/data/tpc_h_data/*.tbl
COPY lineitem from 'lineitem.tbl' DELIMITER '|' CSV;
\timing