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
  (define (unwrap x) (if (identifier? x) (identifier-symbol x) x))
  (eq? (unwrap a) (unwrap b)))

(define (add-scope expr scope)
  (cond
    ((pair? expr)
     (cons
       (add-scope (car expr) scope)
       (add-scope (cdr expr) scope)))
    ((symbol? expr) (make-struct identifier expr scope))
    (else expr)))

;; Macros are bound transformers

(define macro (make-struct-type 'macro 'transformer 'env))
(define (macro-transformer x) (struct-ref x 0 macro))
(define (macro-env x) (struct-ref x 1 macro))
(define (macro? x) (eq? (struct-typeof x) macro))

;; Macro expander

(define (expander func . args)
  ;; Expander state

  (define j 0)
  (define envs ())
  (define (next-timestamp) (set! j (+ j 1)))

  (define (form? expr label)
    (and (pair? expr) (symbol-identifier=? (car expr) label)))

  ;; Lookup + expand macros

  (define (lookup-macro expr)
    (and
      (identifier? (car expr))
      (begin
        (define env (cdr (assv (identifier-scope (car expr)) envs)))
        (define value (env-ref env (identifier-symbol (car expr)) #f))
        (and (macro? value) value))))

  (define (expand expr)
    (cond
      [(pair? expr) (cons (expand-form (car expr)) (expand (cdr expr)))]
      [else expr]))

  (define (expand-form expr)
    (cond
      [(not (pair? expr)) expr]
      [(symbol-identifier=? (car expr) 'quote) expr]
      [(lookup-macro expr) =>
       (lambda (macro)
         (define scope (next-timestamp))
         (set! envs (cons (cons scope (macro-env macro)) envs))
         (expand-form ((macro-transformer macro) expr 0 scope)))]
      [else (cons (expand-form (car expr)) (expand (cdr expr)))]))

  ;; Create gensyms for internal added bindings

  (define (gather-lambda-args formals)
    (if (pair? formals)
      (cons (car formals) (gather-lambda-args (cdr formals)))
      (if (null? formals) () (cons formals ()))))

  (define (add-bindings args current)
    (define (rename s)
      (if (eqv? (identifier-scope s) 0) (identifier-symbol s) (gensym)))
    (append (map (lambda (s) (cons s (rename s))) args) current))

  ;; Unstamp, replacing renamed identifiers with gensyms

  (define (unstamp-identifier id bindings)
    (define sym (identifier-symbol id))
    (cond
      ((eqv? (identifier-scope id) 0) sym)
      ((memq sym '(if define set! begin lambda quote)) sym)
      ((assoc id bindings bound-identifier=?) => cdr)
      (else (list 'env-ref
              (cdr (assv (identifier-scope id) envs))
              (list 'quote sym)))))

  (define (unstamp expr bindings)
    (cond
      [(pair? expr)
       (cons (unstamp-form (car expr) bindings) (unstamp (cdr expr) bindings))]
      [(eq? (struct-typeof expr) identifier)
       (unstamp-identifier expr bindings)]
      [else expr]))

  (define (unstamp-form expr bindings)
    (cond
      [(form? expr 'lambda)
       (begin
        (define new (add-bindings (gather-lambda-args (cadr expr)) bindings))
        (cons 'lambda (unstamp-form (cdr expr) new)))]
      [(pair? expr)
       (cons (unstamp-form (car expr) bindings) (unstamp (cdr expr) bindings))]
      [(eq? (struct-typeof expr) identifier)
       (unstamp-identifier expr bindings)]
      [else expr]))

  ;; Run full expander

  (define (run expr env)
    (set! envs (list (cons 0 env)))
    ;(display 'before:) (newline) (display expr) (newline)
    (set! expr (add-scope expr 0))
    (set! expr (expand-form expr))
    ;(display 'expanded:) (newline) (display expr) (newline)
    (set! expr (unstamp-form expr ()))
    ;(display 'after:) (newline) (display expr) (newline)
    ;(newline)
    expr)

  (apply (env-ref (current-env) func) args))

(define (er-macro-transformer f)
  (lambda (expr caller-env macro-env)
    (define (rename x) (add-scope x macro-env))
    (add-scope
      (f expr rename symbol-identifier=?)
      caller-env)))

(define (ir-macro-transformer f)
  (lambda (expr caller-env macro-env)
    (define (inject x) (add-scope x caller-env))
    (add-scope
      (f expr inject symbol-identifier=?)
      macro-env)))

;; Redefinition of cond, and, or to er-macros

(define or
  (make-struct macro
    (er-macro-transformer
      (lambda (expr rename =?)
        (define name (gensym))
        (define body (cdr expr))
        (define (nested arg rest)
          (if (null? rest)
            arg
            `(begin
              (set! ,name ,arg)
              (if ,name
                ,name
                ,(nested (car rest) (cdr rest))))))
        (if (null? body) #f
          `((lambda (,name) ,(nested (car body) (cdr body))) #f))))
    (current-env)))

(define and
  (make-struct macro
    (er-macro-transformer
      (lambda (expr rename =?)
        (define body (cdr expr))
        (define (nested arg rest)
          (if (null? rest)
            arg
            `(if ,arg ,(nested (car rest) (cdr rest)) #f)))
        (if (null? body) #t (nested (car body) (cdr body)))))
    (current-env)))

(define cond
  (make-struct macro
    (er-macro-transformer
      (lambda (expr rename =?)
        (define name (gensym))
        (define (nested clause rest)
          (if (and (null? rest) (=? (car clause) 'else))
            (cons 'begin (cdr clause))
           `(begin
             (set! ,name ,(car clause))
             (if ,name
               ,(cond
                 [(=? (cadr clause) '=>) (list (caddr clause) name)]
                 [(null? (cdr clause)) name]
                 [else (cons 'begin (cdr clause))])
               ,(if (null? rest)
                 (begin)
                 (nested (car rest) (cdr rest)))))))
        (if (null? (cdr expr))
          (begin)
          `((lambda (,name) ,(nested (cadr expr) (cddr expr))) (begin)))))
    (current-env)))

(define (macroexpand expr env) (expander 'run expr env))

(define (eval expr env)
  (cond
    [(symbol? expr) (env-ref env expr)]
    [(not (pair? expr)) expr]
    [(eq? (car expr) 'define-syntax)
     (eval
       (list 'define
         (cadr expr)
         (list make-struct macro (caddr expr) env))
       env)]
    [else ((compile (macroexpand (qq-expand expr) env) env))]))

(set! early-macros (begin))
(set! define-early-macro (begin))

;(define-early-macro 'and-let*
;  (lambda (body)
;    (define args (car body))
;    (set! body (cdr body))
;    (define name (gensym))
;    (list
;      (list 'lambda (list name)
;        (let loop ((args args))
;          (cond
;            ((null? args) (cons 'begin body))
;            ((identifier? (caar args))
;             `(begin (define . ,(car args)) (if ,(caar args) ,(loop (cdr args)) #f)))
;            (else
;             `(begin (set! ,name ,(caar args)) (if ,name ,(loop (cdr args)) #f))))))
;             `(if ,(caar args) ,(loop (cdr args)) #f))))))
;      #f)))
;

(define-syntax letrec*
  (ir-macro-transformer
    (lambda (expr inject =?)
      `((lambda ()
          ,@(map (lambda (def) (cons 'define def)) (cadr expr))
          . ,(cddr expr))))))

(define-syntax let
  (ir-macro-transformer
    (lambda (expr inject =?)
      (define body (cdr expr))
      (define (inner-lambda body)
        `(lambda ,(map car (car body)) . ,(cdr body)))
      (if (preidentifier? (car body))
        `((letrec* ((,(car body) ,(inner-lambda (cdr body)))) ,(car body))
          . ,(map cadr (cadr body)))
        `(,(inner-lambda body)
          . ,(map cadr (car body)))))))
