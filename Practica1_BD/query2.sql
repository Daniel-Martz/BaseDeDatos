SELECT bookings.book_ref,
       bookings.total_amount,
       Sum (ticket_flights.amount) AS calculated_value
FROM   bookings
       JOIN tickets
         ON bookings.book_ref = tickets.book_ref
       JOIN ticket_flights
         ON tickets.ticket_no = ticket_flights.ticket_no
GROUP  BY bookings.book_ref
ORDER  BY bookings.book_ref ASC; 