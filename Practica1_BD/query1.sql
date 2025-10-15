SELECT f_first.departure_airport,
       Count(DISTINCT b.book_ref) AS num_reservas
FROM   bookings b
       JOIN tickets t
         ON b.book_ref = t.book_ref
       JOIN (SELECT tf.ticket_no,
                    Min(f.scheduled_departure) AS first_departure_time,
                    Max(f.scheduled_departure) AS last_departure_time
             FROM   ticket_flights tf
                    JOIN flights f
                      ON tf.flight_id = f.flight_id
             GROUP  BY tf.ticket_no) ftime
         ON t.ticket_no = ftime.ticket_no
       JOIN flights f_first
         ON f_first.scheduled_departure = ftime.first_departure_time
            AND f_first.flight_id IN (SELECT flight_id
                                      FROM   ticket_flights
                                      WHERE  ticket_no = t.ticket_no)
       JOIN flights f_last
         ON f_last.scheduled_departure = ftime.last_departure_time
            AND f_last.flight_id IN (SELECT flight_id
                                     FROM   ticket_flights
                                     WHERE  ticket_no = t.ticket_no)
WHERE  f_first.departure_airport = f_last.arrival_airport
GROUP  BY f_first.departure_airport
ORDER  BY f_first.departure_airport;