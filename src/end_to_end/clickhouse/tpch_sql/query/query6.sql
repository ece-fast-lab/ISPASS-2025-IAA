SET enable_deflate_qpl_codec = 1, background_pool_size = 1, max_threads = 1;
SELECT
    sum(l_extendedprice * l_discount) AS revenue
FROM
    lineitem
WHERE
    l_shipdate >= DATE '1994-01-01'
    AND l_shipdate < DATE '1994-01-01' + INTERVAL '1' year
    AND l_discount BETWEEN 0.05 AND 0.07
    AND l_quantity < 24;