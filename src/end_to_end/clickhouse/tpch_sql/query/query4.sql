SET enable_deflate_qpl_codec = 1, background_pool_size = 1, max_threads = 1;
WITH ValidLineItems AS (
    SELECT
        l_orderkey
    FROM
        lineitem
    WHERE
        l_commitdate < l_receiptdate
    GROUP BY
        l_orderkey
)
SELECT
    o.o_orderpriority,
    COUNT(*) AS order_count
FROM
    orders o
JOIN
    ValidLineItems vli ON o.o_orderkey = vli.l_orderkey
WHERE
    o.o_orderdate >= DATE '1993-07-01'
    AND o.o_orderdate < DATE '1993-07-01' + INTERVAL '3' MONTH
GROUP BY
    o.o_orderpriority
ORDER BY
    o.o_orderpriority;