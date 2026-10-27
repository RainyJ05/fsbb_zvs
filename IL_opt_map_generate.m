%% 1. 设计参数（按实际系统修改）
Vin_axis = 40:1:80;          % 输入电压
Vo_axis  = 40:1:80;          % 输出电压
Icmd_axis = 0:0.5:20;             % 负载电流指令

fsw = 400e3; %开关频率
gTs = 1/fsw/100; %仿真步长，取开关频率的1/100

Ts = 1/fsw;                   % 开关周期 400kHz
T2_axis = linspace(0, Ts, 1000); % T2 候选值，通常 0 ~ Ts/2

Coss = 364e-12;                 % 开关管输出电容
tdead = 100e-9;                 % 死区时间
L = 1e-6;                       % 电感
I0 = 1;                       % 需要你根据电路确定，这里仅为占位

%% 2. 预分配表格
nVin  = length(Vin_axis);
nVo   = length(Vo_axis);
nLoad = length(Icmd_axis);

T1_table   = NaN(nVin, nVo, nLoad);
T2_table   = NaN(nVin, nVo, nLoad);
T3_table   = NaN(nVin, nVo, nLoad);
T4_table   = NaN(nVin, nVo, nLoad);
Irms_table = NaN(nVin, nVo, nLoad);

%% 3. 三层循环 + 内层搜索最优 T2
for iVin = 1:nVin
    Vin = Vin_axis(iVin);

    for iVo = 1:nVo
        Vo = Vo_axis(iVo);

        Izvs = 2*Coss*max(Vin,Vo)/tdead;

        for iLoad = 1:nLoad
            Icmd = Icmd_axis(iLoad);

            bestIrms2 = inf;
            bestT1 = NaN; bestT2 = NaN; bestT3 = NaN; bestT4 = NaN;

            for T2 = T2_axis

                disc = I0^2*L^2 ...
                     + 2*Icmd*Ts*Vo*L ...
                     + Vin*Vo*T2^2;

                if disc < 0
                    continue;
                end

                T1 = (I0*L - Vin*T2 + sqrt(disc))/Vin;

                I1 = -I0 + Vin*T1/L;
                I2 = I1 + (Vin-Vo)*T2/L;

                T3 = Vin/Vo*(T1+T2)-T2;
                T4 = Ts - T1 - T2 - T3;

                if min([T1,T2,T3,T4]) < 0
                    continue;
                end

                if min([I0,I1,I2]) < Izvs
                    continue;
                end

                Irms2 = ...
                    (T1*(I0^2-I0*I1+I1^2) ...
                    + T2*(I1^2+I1*I2+I2^2) ...
                    + T3*(I2^2-I0*I2+I0^2))/(3*Ts) ...
                    + I0^2*T4/Ts;

                if Irms2 < bestIrms2
                    bestIrms2 = Irms2;
                    bestT1 = T1;
                    bestT2 = T2;
                    bestT3 = T3;
                    bestT4 = T4;
                end
            end

            T1_table(iVin,iVo,iLoad) = bestT1/Ts;
            T2_table(iVin,iVo,iLoad) = bestT2/Ts;
            T3_table(iVin,iVo,iLoad) = bestT3/Ts;
            T4_table(iVin,iVo,iLoad) = bestT4/Ts;
            Irms_table(iVin,iVo,iLoad) = sqrt(bestIrms2);

        end
    end
end

%% 4. 保存结果
save('ZVS_LUT.mat', 'Vin_axis', 'Vo_axis', 'Icmd_axis', ...
     'T1_table', 'T2_table', 'T3_table', 'T4_table', 'Irms_table', ...
     'T2_axis', 'Ts', 'Coss', 'tdead', 'L', 'I0', 'gTs', 'fsw');
