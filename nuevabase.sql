CREATE TABLE seats_new AS
SELECT f.flight_id, s.seat_no, s.fare_conditions
FROM flights f
JOIN seats s ON f.aircraft_code = s.aircraft_code;

ALTER TABLE seats_new
ADD CONSTRAINT seats_new_pkey PRIMARY KEY (flight_id, seat_no);

ALTER TABLE seats_new
ADD CONSTRAINT seats_new_flight_id_fkey
FOREIGN KEY (flight_id) REFERENCES flights(flight_id);

DROP TABLE seats CASCADE;
ALTER TABLE seats_new RENAME TO seats;

