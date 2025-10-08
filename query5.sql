SELECT DISTINCT t.book_ref, 
                tf.flight_id 
FROM   tickets t
       JOIN ticket_flights tf 
         ON t.ticket_no = tf.ticket_no 
WHERE  NOT EXISTS (SELECT 1 
                   FROM   boarding_passes bp 
                   WHERE  bp.flight_id = tf.flight_id 
                          AND bp.ticket_no = t.ticket_no) 
ORDER  BY t.book_ref, 
          tf.flight_id; 