set_languages("gnu23", "gnuxx23")

-- Debug build
add_cxflags("-Og", "-g3")

add_cxflags("-Wall", "-Wextra", "-march=native")
add_cxflags("-Iinclude")

target("scheme")
--     set_kind("shared")
    set_kind("static")
    add_files("src/*.cpp|main.cpp", "src/builtin/*.cpp")

target("scheme-repl")
    set_kind("binary")
    add_deps("scheme")
    add_files("src/main.cpp")

for _, file in ipairs(os.files("tests/test_*.cpp")) do
    local name = path.basename(file)
    target(name)
        set_kind("binary")
        set_default(false)
        add_deps("scheme")
        add_files(file)
        add_tests("default")
end
