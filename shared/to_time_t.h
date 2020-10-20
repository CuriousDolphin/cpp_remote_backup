#pragma once
template <typename TP>
int to_timestamp(TP tp)
{
    using namespace std::chrono;
    auto sctp = time_point_cast<system_clock::duration>(tp - TP::clock::now() + system_clock::now());
    long int t = static_cast<long int>(system_clock::to_time_t(sctp));
    return t;
}