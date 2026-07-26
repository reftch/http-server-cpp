#ifndef CPU_H
#define CPU_H

class Cpu {
   public:
    Cpu();

    // Returns system CPU usage percentage (0.0 - 100.0)
    // Needs to be called periodically.
    double usage();

   private:
    struct CpuTimes {
        long long idle;
        long long total;
    };

    CpuTimes getCpuTimes();

    CpuTimes previous_;
    bool initialized_;
};

#endif  // CPU_H