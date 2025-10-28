(define (iota count start)
  (if (> count 0)
    (cons start (iota (- count 1) (+ start 1)))
    '()))

(define (swap! v i j)
  (define tmp (vector-ref v i))
  (vector-set! v i (vector-ref v j))
  (vector-set! v j tmp))

(define (each-permutation f v)
  (define l (vector-length v))
  (let outer ([i 0])
    (if (< i l)
      (begin
       (let inner ([j (+ i 1)])
         (if (< j l)
           (begin
             (swap! v i j)
             (outer (+ i 1))
             (swap! v i j)
             (inner (+ j 1)))))
       (outer (+ i 1)))
      (f v))))

(define (flip-head! n vec)
  (let loop ([i 0] [j (- n 1)])
    (if (< i j)
      (begin
        (swap! vec i j)
        (loop (+ i 1) (- j 1))))))

(define (fannkuch-vec vec)
  (set! vec (vector-copy vec))
  (let loop ([n 0])
    (if (eqv? (vector-ref vec 0) 1) n
      (begin
        (flip-head! (vector-ref vec 0) vec)
        (loop (+ n 1))))))

(define (fannkuch-max vec)
  (define m 0)
  (each-permutation
    (lambda (p)
      (define n (fannkuch-vec p))
      (if (> n m) (set! m n)))
    vec)
  m)

(display (fannkuch-max (list->vector (iota 8 1))))
(newline)
