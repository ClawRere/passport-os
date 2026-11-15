#pragma once
#include "apps/app_base.h"

// Lives on the existing ADC task's stack. Resolve gestures at sample time,
// before the input queue can be delayed by network/audio teardown. Only OK
// clicks wait; UP/DOWN press/release (including push-to-talk) stay immediate.
//
// Three-event model:
//   BTN_EVT_CLICK       – single tap (no follow-up within kDoubleUs)
//   BTN_EVT_DOUBLE      – two fast taps
//   BTN_EVT_TRIPLE      – three fast taps (only recognized immediately after DOUBLE)
class OkGesture {
public:
    // 300 ms made every confirmation feel stalled. A 180 ms inter-press
    // window retains exclusive double-clicks while shortening single clicks.
    static constexpr int64_t kDoubleUs = 180000;
    // Triple detection: a third tap must arrive within this window after
    // the DOUBLE has been emitted. Longer than kDoubleUs so a slow middle
    // tap does not accidentally form a triple.
    static constexpr int64_t kTripleGapUs = 280000;

    template<class Emit>
    void poll(int64_t now, passport_app_id_t app, Emit emit) {
        if (pending_ && app != owner_) {
            pending_ = false;
            discard_tail_ = second_down_;
            second_down_ = false;
            // App switched while pressing – cancels any pending triple hope.
            triple_chain_ = false;
        }
        if (pending_ && !second_down_ && now - released_at_ >= kDoubleUs) flush(emit);
        // Time-out for triple chain: if DOUBLE was just emitted and no third tap
        // lands within kTripleGapUs, forget about tripling.
        if (triple_chain_ && now - triple_at_us_ >= kTripleGapUs)
            triple_chain_ = false;
    }

    template<class Emit>
    void event(uint8_t button, app_btn_event_t event, uint16_t mv,
               int64_t now, passport_app_id_t app, Emit emit) {
        poll(now, app, emit);
        if (button != BSP_BTN_OK) {
            // A different key ends the double-click opportunity. Preserve
            // input order instead of applying the pending OK to a later row.
            if (event == BTN_EVT_PRESS) flush(emit);
            emit(button, event, mv);
            return;
        }
        if (event == BTN_EVT_PRESS) {
            discard_tail_ = false;
            second_down_ = pending_;
            owner_ = app;
            // A fresh press while a triple-chain is active means the user may
            // still be trying to land a third tap – don't emit yet.
            emit(button, event, mv); // wake screen immediately, without a click
        } else if (discard_tail_) {
            if (event == BTN_EVT_RELEASE) discard_tail_ = false;
        } else if (event == BTN_EVT_CLICK) {
            if (app != owner_) {
                pending_ = second_down_ = false;
                discard_tail_ = true;
                triple_chain_ = false;
                return; // foreground changed while the button was held
            }
            if (second_down_) {
                // This click completes the pair: either DOUBLE or (if preceded
                // by a DOUBLE emission) TRIPLE.
                pending_ = second_down_ = false;
                discard_tail_ = true;
                if (triple_chain_) {
                    // Third tap arrived in time – send TRIPLE, not DOUBLE again.
                    triple_chain_ = false;
                    emit(button, BTN_EVT_TRIPLE, mv);
                } else {
                    // Fresh double – arm triple detection for the next tap.
                    triple_chain_ = true;
                    triple_at_us_ = now;
                    emit(button, BTN_EVT_DOUBLE, mv);
                }
            } else {
                // First tap of a potential sequence.
                pending_ = true;
                owner_ = app;
                released_at_ = now;
                mv_ = mv;
            }
        } else if (event == BTN_EVT_LONG) {
            // Single then hold means Back/power, never a delayed click on the
            // parent page. LONG_UP/RELEASE keep their existing semantics.
            pending_ = second_down_ = false;
            triple_chain_ = false;
            emit(button, event, mv);
        } else if (event != BTN_EVT_RELEASE || !pending_) {
            emit(button, event, mv);
        }
    }

private:
    template<class Emit> void flush(Emit emit) {
        if (!pending_) return;
        pending_ = second_down_ = false;
        triple_chain_ = false;
        emit(BSP_BTN_OK, BTN_EVT_CLICK, mv_);
        emit(BSP_BTN_OK, BTN_EVT_RELEASE, mv_);
    }
    int64_t released_at_ = 0;
    passport_app_id_t owner_ = APP_ID_COUNT;
    uint16_t mv_ = 0;
    bool pending_ = false, second_down_ = false, discard_tail_ = false;
    // True once a DOUBLE has just been emitted; cleared when a third tap lands
    // (converting to TRIPLE) or the gap expires.
    bool triple_chain_ = false;
    int64_t triple_at_us_ = 0;
};
