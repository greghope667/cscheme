(define (check y positions)
  (define (loop px py rest)
    (cond
      ((= y (+ py px)) #f)
      ((= y (- py px)) #f)
      ((null? rest) #t)
      (#t (loop (+ px 1) (car rest) (cdr rest)))))
  (if (null? positions) #t (loop 1 (car positions) (cdr positions))))

(define (queens current checked remaining)
  (if (null? remaining)
    (if (null? checked)
      1;(begin (display current) (newline) 1)
      0)
    (+
      (if (check (car remaining) current)
        (queens (cons (car remaining) current) '() (append checked (cdr remaining)))
        0)
      (queens current (cons (car remaining) checked) (cdr remaining)))))

(define (iota count start)
  (if (> count 0)
    (cons start (iota (- count 1) (+ start 1)))
    '()))

(define (nqueens n) (queens '() '() (iota n 1)))

(display (map nqueens (iota 10 1)))
(newline)
