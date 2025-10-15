WITH tabla_def
     AS (WITH tabla
              AS (SELECT *,
                         actual_arrival - scheduled_arrival AS retraso
                  FROM   flights)
         SELECT flight_no,
                Avg(retraso) AS retraso_medio
          FROM   tabla
          GROUP  BY flight_no)
SELECT flight_no,
       retraso_medio
FROM   tabla_def
WHERE  retraso_medio = (SELECT Max(retraso_medio)
                        FROM   tabla_def)