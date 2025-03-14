\timing
-- Q17  Small-Quantity-Order Revenue Query
-- select
--   sum(l_extendedprice) / 7.0 as avg_yearly
-- from
--   lineitem,
--   part
-- where
--   p_partkey = l_partkey
--   and p_brand = 'Brand#23'
--   and p_container = 'MED BOX'
--   and l_quantity < (
--     select
--       0.2 * avg(l_quantity)
--     from
--       lineitem
--     where
--       l_partkey = p_partkey
--   );
WITH AvgQuantity AS (
    SELECT
        l_partkey,
        AVG(l_quantity) * 0.2 AS avg_quantity
    FROM
        lineitem
    GROUP BY
        l_partkey
)
SELECT
    SUM(l.l_extendedprice) / 7.0 AS avg_yearly
FROM
    lineitem l
JOIN
    part p ON p.p_partkey = l.l_partkey
JOIN
    AvgQuantity aq ON l.l_partkey = aq.l_partkey
WHERE
    p.p_brand = 'Brand#23'
    AND p.p_container = 'MED BOX'
    AND l.l_quantity < aq.avg_quantity;
\timing