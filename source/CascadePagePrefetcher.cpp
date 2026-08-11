#include <CascadePagePrefetcher.hpp>

CascadePagePrefetcher::CascadePagePrefetcher(manga::IMangaSource *source) : source(source) {
    this->worker = std::thread([this]() {
        this->WorkerLoop();
    });
}

CascadePagePrefetcher::~CascadePagePrefetcher() {
    this->Stop();
}

void CascadePagePrefetcher::RequestAhead(const u32 index) {
    std::lock_guard<std::mutex> lock(this->mutex);
    if (this->stopping || (this->known.find(index) != this->known.end())) {
        return;
    }

    this->known.insert(index);
    this->pending.push_back(index);
    this->cv.notify_all();
}

pu::sdl2::Surface CascadePagePrefetcher::TakeDecoded(const u32 index) {
    std::unique_lock<std::mutex> lock(this->mutex);
    if (this->known.find(index) == this->known.end()) {
        this->known.insert(index);
        this->pending.push_back(index);
        this->cv.notify_all();
    }

    pu::sdl2::Surface result = nullptr;
    this->cv.wait(lock, [this, index, &result]() {
        for (size_t i = 0; i < this->decoded.size(); i++) {
            if (this->decoded.at(i).index == index) {
                result = this->decoded.at(i).surface;
                this->decoded.erase(this->decoded.begin() + i);
                return true;
            }
        }
        return this->stopping;
    });

    this->known.erase(index);
    return result;
}

void CascadePagePrefetcher::Stop() {
    {
        std::lock_guard<std::mutex> lock(this->mutex);
        if (this->stopping) {
            return;
        }
        this->stopping = true;
    }
    this->cv.notify_all();
    if (this->worker.joinable()) {
        this->worker.join();
    }

    std::lock_guard<std::mutex> lock(this->mutex);
    for (auto &page : this->decoded) {
        if (page.surface != nullptr) {
            SDL_FreeSurface(page.surface);
        }
    }
    this->decoded.clear();
    this->pending.clear();
    this->known.clear();
}

void CascadePagePrefetcher::WorkerLoop() {
    while (true) {
        u32 index = 0;
        {
            std::unique_lock<std::mutex> lock(this->mutex);
            this->cv.wait(lock, [this]() {
                return this->stopping || !this->pending.empty();
            });
            if (this->stopping) {
                return;
            }
            index = this->pending.front();
            this->pending.erase(this->pending.begin());
        }

        const auto data = this->source->ReadPage(index);
        pu::sdl2::Surface surface = nullptr;
        if (!data.empty()) {
            surface = IMG_Load_RW(SDL_RWFromConstMem(data.data(), data.size()), 1);
        }

        {
            std::lock_guard<std::mutex> lock(this->mutex);
            this->decoded.push_back({index, surface});
        }
        this->cv.notify_all();
    }
}
