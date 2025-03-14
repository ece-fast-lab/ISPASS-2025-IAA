\timing
--Q2
-- select
--   s_acctbal,
--   s_name,
--   n_name,
--   p_partkey,
--   p_mfgr,
--   s_address,
--   s_phone,
--   s_comment
-- from
--   part,
--   supplier,
--   partsupp,
--   nation,
--   region
-- where
--   p_partkey = ps_partkey
--   and s_suppkey = ps_suppkey
--   and p_size = 15
--   and p_type like '%BRASS'
--   and s_nationkey = n_nationkey
--   and n_regionkey = r_regionkey
--   and r_name = 'EUROPE'
--   and ps_supplycost = (
--     select
--       min(ps_supplycost)
--     from
--       partsupp,
--       supplier,
--       nation,
--       region
--     where
--       p_partkey = ps_partkey
--       and s_suppkey = ps_suppkey
--       and s_nationkey = n_nationkey
--       and n_regionkey = r_regionkey
--       and r_name = 'EUROPE'
--     )
-- order by
--   s_acctbal desc,
--   n_name,
--   s_name,
--   p_partkey;
WITH MinSupplyCost AS (
    SELECT
        ps_partkey,
        MIN(ps_supplycost) AS min_supplycost
    FROM
        partsupp ps
    JOIN
        supplier s ON ps.ps_suppkey = s.s_suppkey
    JOIN
        nation n ON s.s_nationkey = n.n_nationkey
    JOIN
        region r ON n.n_regionkey = r.r_regionkey
    WHERE
        r.r_name = 'EUROPE'
    GROUP BY
        ps_partkey
)
SELECT
    s.s_acctbal,
    s.s_name,
    n.n_name,
    p.p_partkey,
    p.p_mfgr,
    s.s_address,
    s.s_phone,
    s.s_comment
FROM
    part p
JOIN
    partsupp ps ON p.p_partkey = ps.ps_partkey
JOIN
    supplier s ON s.s_suppkey = ps.ps_suppkey
JOIN
    nation n ON s.s_nationkey = n.n_nationkey
JOIN
    region r ON n.n_regionkey = r.r_regionkey
JOIN
    MinSupplyCost msc ON ps.ps_partkey = msc.ps_partkey AND ps.ps_supplycost = msc.min_supplycost
WHERE
    p.p_size = 15
    AND p.p_type LIKE '%BRASS'
    AND r.r_name = 'EUROPE'
ORDER BY
    s.s_acctbal DESC,
    n.n_name,
    s.s_name,
    p.p_partkey;
\timing