-- select
--   s_name,
--   s_address
-- from
--   supplier, nation
-- where
--   s_suppkey in (
--     select
--       ps_suppkey
--     from
--       partsupp
--     where
--       ps_partkey in (
--         select
--           p_partkey
--         from
--           part
--         where
--           p_name like 'forest%'
--         )
--       and ps_availqty > (
--         select
--           0.5 * sum(l_quantity)
--         from
--           lineitem
--         where
--           l_partkey = ps_partkey
--           and l_suppkey = ps_suppkey
--           and l_shipdate >= '1994-01-01'
--           and l_shipdate < '1995-01-01'
--         )
--     )
--   and s_nationkey = n_nationkey
--   and n_name = 'CANADA'
-- order by
--   s_name

\timing
SELECT
  s_name,
  s_address
FROM
  supplier
  JOIN nation ON supplier.s_nationkey = nation.n_nationkey
  JOIN partsupp ON supplier.s_suppkey = partsupp.ps_suppkey
  JOIN part ON partsupp.ps_partkey = part.p_partkey
  LEFT JOIN (
    SELECT
      l_partkey,
      l_suppkey,
      0.5 * SUM(l_quantity) AS threshold
    FROM
      lineitem
    WHERE
      l_shipdate >= '1994-01-01'
      AND l_shipdate < '1995-01-01'
    GROUP BY
      l_partkey,
      l_suppkey
  ) AS lineitem_threshold
    ON partsupp.ps_partkey = lineitem_threshold.l_partkey
    AND partsupp.ps_suppkey = lineitem_threshold.l_suppkey
WHERE
  part.p_name LIKE 'forest%'
  AND partsupp.ps_availqty > COALESCE(lineitem_threshold.threshold, 0)
  AND nation.n_name = 'CANADA'
ORDER BY
  s_name;
\timing