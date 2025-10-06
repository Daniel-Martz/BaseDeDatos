SELECT f.arrival_airport, 
       Count(*) AS num_pasajeros_recibidos 
FROM   boarding_passes bp 
       JOIN ticket_flights tf 
         ON ( bp.flight_id = tf.flight_id 
              AND bp.ticket_no = tf.ticket_no ) 
       JOIN flights f 
         ON tf.flight_id = f.flight_id 
GROUP  BY f.arrival_airport 
ORDER  BY num_pasajeros_recibidos ASC 