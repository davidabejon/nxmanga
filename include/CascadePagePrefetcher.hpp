#pragma once

#include <pu/Plutonium>
#include <manga/IMangaSource.hpp>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <unordered_set>
#include <vector>

// Decodes cascade-mode manga pages on a single background thread.
//
// A full JPEG/PNG/WEBP decode takes tens of milliseconds for a typical
// manga page; doing that synchronously on the main thread (as
// MangaViewerLayout used to) blocks input/rendering for that long every
// time a new page scrolls into range, which is felt as a small stutter on
// almost every scroll tick. Decoding to an SDL_Surface is pure CPU work and
// safe to do off-thread; only the final upload to a GPU texture
// (pu::ui::render::ConvertToTexture) has to happen on the renderer's own
// thread, so that part stays the caller's responsibility.
//
// Every read of the underlying IMangaSource happens on this class's worker
// thread, and only there: some sources (e.g. CbzMangaSource) keep a single
// stateful archive handle that isn't safe to read from two threads at
// once, so the caller must never call source->ReadPage() itself while a
// CascadePagePrefetcher for that same source is running.
class CascadePagePrefetcher {
    public:
        // source is read only from the worker thread for as long as this
        // object is alive; the caller must keep it alive at least that
        // long, and must not read from it itself in the meantime.
        explicit CascadePagePrefetcher(manga::IMangaSource *source);
        ~CascadePagePrefetcher();

        // Queues index for background decoding, unless it's already
        // queued, being decoded, or already decoded and waiting to be
        // collected via TakeDecoded. A no-op once Stop() has been called.
        void RequestAhead(const u32 index);

        // Returns index's decoded surface (the caller takes ownership, and
        // must eventually pu::ui::render::ConvertToTexture or
        // SDL_FreeSurface it), or nullptr if the page couldn't be read or
        // decoded. If it hasn't been requested yet, requests it now and
        // blocks until it's ready — the common case is that it was already
        // prefetched well ahead of time via RequestAhead, so this returns
        // immediately.
        pu::sdl2::Surface TakeDecoded(const u32 index);

        // Non-blocking counterpart to TakeDecoded: if index has already
        // finished decoding, fills out_surface (caller takes ownership, same
        // as TakeDecoded) and returns true. Otherwise returns false
        // immediately without waiting and without queuing a request for it
        // — the caller is expected to have already requested it (via
        // RequestAhead or a prior TakeDecoded) if it wants one. Lets a
        // render callback opportunistically drain already-decoded pages a
        // little ahead of when the caller actually needs them.
        bool TryTakeDecoded(const u32 index, pu::sdl2::Surface &out_surface);

        // Stops the worker thread (waiting for whatever it's mid-decoding
        // to finish) and frees every decoded surface still waiting to be
        // collected. Safe to call more than once; called automatically by
        // the destructor if not already stopped.
        void Stop();

    private:
        struct DecodedPage {
            u32 index;
            pu::sdl2::Surface surface;
        };

        void WorkerLoop();

        manga::IMangaSource *source;
        std::thread worker;
        std::mutex mutex;
        std::condition_variable cv;
        bool stopping = false;
        std::vector<u32> pending;
        // Indices that are queued, currently being decoded, or decoded and
        // sitting in `decoded` awaiting TakeDecoded — kept in one set so
        // RequestAhead never queues the same index twice.
        std::unordered_set<u32> known;
        std::vector<DecodedPage> decoded;
};
