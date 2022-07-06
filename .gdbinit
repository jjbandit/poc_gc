tui enable

set follow-fork-mode child

# b complain_loudly_on_segfault_handler
# b Str::slice
# b Str::replace
# b CommandParser::_ParseRedirectList
# b Str::rstrip

b Allocate
d 1
start
