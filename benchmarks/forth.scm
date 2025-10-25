(define (dup ip rsp tos psp)
  ((car ip) (cdr ip) rsp tos (cons tos psp)))

(define (pop ip rsp tos psp)
  ((car ip) (cdr ip) rsp (car psp) (cdr psp)))

(define (swap ip rsp tos psp)
  ((car ip) (cdr ip) rsp (car psp) (cons tos (cdr psp))))

(define (add ip rsp tos psp)
  ((car ip) (cdr ip) rsp (+ (car psp) tos) (cdr psp)))

(define (sub ip rsp tos psp)
  ((car ip) (cdr ip) rsp (- (car psp) tos) (cdr psp)))

(define (gte ip rsp tos psp)
  ((car ip) (cdr ip) rsp (>= (car psp) tos) (cdr psp)))

(define (lit ip rsp tos psp)
  ((cadr ip) (cddr ip) rsp (car ip) (cons tos psp)))

(define (branchf ip rsp tos psp)
  (set! ip (list-tail ip (if tos 1 (car ip))))
  ((car ip) (cdr ip) rsp (car psp) (cdr psp)))

(define (exit ip rsp tos psp)
  (if (null? rsp)
    tos
    ((caar rsp) (cdar rsp) (cdr rsp) tos psp)))

(define (colon . code)
  (set! code (append code (list exit)))
  (lambda (ip rsp tos psp)
    ((car code) (cdr code) (cons ip rsp) tos psp)))

(define (forth . code)
  (set! code (append code (list exit)))
  (newline)
  ((car code) (cdr code) '() #f '()))


(define fibrec #f)
(define (fib . x) (apply fibrec x))
;(define (fib a b c d) (fibrec a b c d))

(set! fibrec
  (colon
    dup lit 2 gte branchf 12
      dup lit 2 sub fib
      swap lit 1 sub fib
      add))

(display (forth lit 25 fib)) (newline)
