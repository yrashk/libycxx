// [stoptoken.concepts]/4: stoppable_token / unstoppable_token; [stoptoken.general]/1: "The
// class stop_token models the concept stoppable_token"; [stoptoken.never]: never_stop_token
// models unstoppable_token, its stop_requested / stop_possible are static constexpr and false;
// [stoptoken.general]: callback_type; [stoptoken.inplace.general]: inplace_stop_token models stoppable_token. [thread.stoptoken.syn]:
// stop_callback_for_t<T, CallbackFn> is typename T::template callback_type<CallbackFn>.
#include <stop_token>
#include <type_traits>

static_assert(std::stoppable_token<std::stop_token>);
static_assert(!std::unstoppable_token<std::stop_token>);
static_assert(std::stoppable_token<std::never_stop_token>);
static_assert(std::unstoppable_token<std::never_stop_token>);
static_assert(std::stoppable_token<std::inplace_stop_token>);
static_assert(!std::unstoppable_token<std::inplace_stop_token>);
static_assert(!std::stoppable_token<int>);
static_assert(!std::stoppable_token<std::stop_source>);

static_assert(std::is_same_v<std::stop_token::callback_type<void (*)()>, std::stop_callback<void (*)()>>);
static_assert(std::is_same_v<std::stop_callback_for_t<std::stop_token, void (*)()>, std::stop_callback<void (*)()>>);
static_assert(std::is_same_v<std::stop_callback_for_t<std::inplace_stop_token, void (*)()>,
                             std::inplace_stop_callback<void (*)()>>);
static_assert(!std::never_stop_token::stop_possible());
static_assert(!std::never_stop_token::stop_requested());
static_assert(std::never_stop_token() == std::never_stop_token());
static_assert(std::is_constructible_v<std::stop_callback_for_t<std::never_stop_token, void (*)()>,
                                      std::never_stop_token, void (*)()>);
