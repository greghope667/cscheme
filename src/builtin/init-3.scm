;; Macro expander

;; Identifiers are symbols tagged with a scope
(define identifier (make-struct-type 'identifier 'symbol 'scope))
(define (identifier-symbol x) (struct-ref x 0 identifier))
(define (identifier-scope x) (struct-ref x 1 identifier))

(define (identifier? x)
  (eq? (struct-typeof x) identifier))

(define (preidentifier? x)
  (or (symbol? x) (identifier? x)))

(define (bound-identifier=? a b)
  (and (eq? (identifier-symbol a) (identifier-symbol b))
    (eqv? (identifier-scope a) (identifier-scope b))))

; http://www.phyast.pitt.edu/~micheles/scheme/scheme30.html
; Standard requires (or heavily implies) using free-identifier=?
; for macros. I find that a bit of an odd choice personally, so
; use this instead.

(define (symbol-identifier=? a b)
  (define (unwrap? x) (if (identifier? x) (identifier-symbol x) x))
  (eq? (unwrap a) (unwrap b)))

;; Macros are bound transformers

(define macro (make-struct-type 'macro 'transformer 'env))
(define (macro-transformer x) (struct-ref x 0 macro))
(define (macro-env x) (struct-ref x 1 macro))
(define (macro? x) (eq? (struct-typeof x) macro))

;; Macro expander

(define j 0)
(define (reset-timestamp) (set! j 0))
(define (next-timestamp) (set! j (+ j 1)))

(define envs ())

(define (add-scope expr scope)
  (cond
    ((pair? expr
      (cons
        (add-scope (car expr) scope)
        (add-scope (cdr expr) scope))))

    ((symbol? expr) (make-struct identifier expr scope))
    (else expr)))

(define (form? expr label)
  (and (pair? expr) (symbol-identifier=? (car expr) label)))

(define (lookup-macro expr)
  (and
    (pair? expr)
    (identifier? (car expr))
    (begin
      (define env (cdr (assv (identifier-scope (car expr)) envs)))
      (define value (env-ref env (identifier-symbol (car expr)) #f))
      (and (macro? value) value))))

(define (expand expr)
  (cond
    ((not (pair? expr)) expr)
    ((lookup-macro expr) =>
      (lambda (macro)
        (define scope (next-timestamp))
        (set! envs (cons (cons scope (macro-env macro)) envs))
        (expand ((macro-transformer macro) expr 0 scope))))
    (else (map expand expr))))

(define (new-bindings args current)
  (define (rename s)
    (if (eqv? (identifier-scope s) 0) (identifier-symbol s) (gensym)))
  (append (map (lambda (s) (cons s (rename s))) args) current))

(define (unstamp-identifier id bindings)
  (define sym (identifier-symbol id))
  (cond
    ((eqv? (identifier-scope id) 0) sym)
    ((memq sym '(if define set! begin lambda)) sym)
    ((assoc id bindings bound-identifier=?) => cdr)
    (else (list 'env-ref
            (cdr (assv (identifier-scope id) envs))
            sym))))

(define (unstamp expr bindings)
  (cond
    ((form? expr 'lambda
      (begin
        (define new (new-bindings (cadr expr) bindings))
        (cons 'lambda (unstamp (cdr expr) new)))))
    ((pair? expr
      (map (lambda (v) (unstamp v bindings)) expr)))
    ((eq? (struct-typeof expr) identifier)
     (unstamp-identifier expr bindings))
    (else expr)))


(define (macroexpand expr env)
  (set! envs (list (cons 0 env)))
  (set! expr (add-scope expr (reset-timestamp)))
  (set! expr (expand expr))
  (set! expr (unstamp expr ()))
  expr)


;;(define (eval expr env)
;;  (cond
;;    ((symbol? expr) (env-ref env expr))
;;    ((not (pair? expr)) expr)
;;    ((form? expr 'define-syntax
;;      (eval
;;        (list 'define
;;          (cadr expr)
;;          (list make-struct macro (caddr expr) env))
;;        env)))
;;    (else ((compile (macroexpand (qq-expand expr) env) env)))))
