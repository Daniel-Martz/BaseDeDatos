WITH tabla1
     AS (SELECT flights.flight_id,
                Count(*) AS asientos_vacios
         FROM   flights
                JOIN seats
                  ON flights.aircraft_code = seats.aircraft_code
                LEFT JOIN boarding_passes
                       ON seats.seat_no = boarding_passes.seat_no
                          AND boarding_passes.flight_id = flights.flight_id
         WHERE  boarding_passes.seat_no IS NULL
         GROUP  BY flights.flight_id)
SELECT *
FROM   tabla1
WHERE  asientos_vacios = (SELECT Max(asientos_vacios)
                          FROM   tabla1) 