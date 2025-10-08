ALTER TABLE seats DROP COLUMN aircrafts_code CASCADE;
ALTER TABLE seats ADD flight_id serial;
ALTER TABLE seats ADD FOREIGN KEY (flight_id) REFFERENCES flights(flight_id);