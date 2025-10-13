(define (create-tree depth)
  (if (eqv? depth 0)
    (cons '() '())
    (cons (create-tree (- depth 1)) (create-tree (- depth 1)))))

(define (check tree)
  (if (null? (car tree))
    1
    (+ (check (car tree)) (check (cdr tree)) 1)))

(define (create-and-check depth)
  (display (check (create-tree depth)))
  (newline))

(define (iota count start)
  (if (> count 0)
    (cons start (iota (- count 1) (+ start 1)))
    '()))

(define long-lived-tree (create-tree 17))

(map create-and-check (iota 16 0))

(display (check long-lived-tree))
(newline)
