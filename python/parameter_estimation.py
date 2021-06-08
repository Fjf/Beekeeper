import re
import subprocess

import numpy as np
import optuna
from matplotlib import pyplot as plt
from optuna.structs import TrialState


class Configuration(object):
    def __init__(self, params=None, e=1):
        self.algorithm = "mm"
        self.mcts_constant = 0
        self.mcts_prioritization = 0
        self.time = 0.01
        self.first_play_urgency = 0
        self.evaluation_function = e
        self.params = params

    def to_cmd(self, player1=True):
        params = []
        if self.params:
            params = ["-m", self.params[0], "-q", self.params[1], "-u", self.params[2]]

        if player1:
            prio = "-P" if self.mcts_prioritization is True else ""
            fpu = "-F" if self.first_play_urgency is True else ""
            result = ["-A", self.algorithm, prio, fpu, "-C", self.mcts_constant, "-T", self.time, "-E",
                    self.evaluation_function] + params
        else:
            prio = "-p" if self.mcts_prioritization is True else ""
            fpu = "-f" if self.first_play_urgency is True else ""
            result = ["-a", self.algorithm, prio, fpu, "-c", self.mcts_constant, "-t", self.time, "-e",
                    self.evaluation_function] + params

        return [str(e) for e in result]

    def __repr__(self):
        return "%s|%s|%d|%s|%d|%s" % (
            self.algorithm, self.mcts_constant, self.mcts_prioritization, self.time, self.first_play_urgency,
            self.evaluation_function)


def playout(p1: Configuration, p2: Configuration):
    base = ["../c/cmake-build-debug/hive_run"]
    command = base + p1.to_cmd() + p2.to_cmd(False)
    # print(" ".join(command))
    result = subprocess.check_output(command).decode("utf-8")
    for line in result.split("\n"):
        if "won" in line:
            if "1" in line:
                return 1
            elif "2" in line:
                return 0
            else:
                return 0.5

    return 0.5


def objective(trial):
    base = Configuration()

    m = trial.suggest_float('movement', 0, 10)
    q = trial.suggest_float('queen', 0, 10)
    u = trial.suggest_float('unused_tiles', 0, 10)

    n_playouts = 20
    result = 0
    for i in range(n_playouts // 2):
        result += playout(base, Configuration((m, q, u), e=2))
        result += (1 - playout(Configuration((m, q, u), e=2), base))

    return result / n_playouts


def main():
    study = optuna.create_study(direction='maximize', storage="sqlite:///bpe_intermediate.sqlite")
    study.optimize(objective, n_trials=100)

    pruned_trials = study.get_trials(deepcopy=False, states=[TrialState.PRUNED])
    complete_trials = study.get_trials(deepcopy=False, states=[TrialState.COMPLETE])

    print("Study statistics: ")
    print("  Number of finished trials: ", len(study.trials))
    print("  Number of pruned trials: ", len(pruned_trials))
    print("  Number of complete trials: ", len(complete_trials))

    print("Best trial:")
    trial = study.best_trial

    print("  Value: ", trial.value)

    print("  Params: ")
    for key, value in trial.params.items():
        print("    {}: {}".format(key, value))


def plot():
    data = """[I 2021-06-03 14:26:58,748] Trial 0 finished with value: 0.6 and parameters: {'movement': 6.719874239603321, 'queen': 2.1952962327067427, 'unused_tiles': 8.876478216715933}. Best is trial 0 with value: 0.6.
[I 2021-06-03 14:27:07,997] Trial 1 finished with value: 0.1 and parameters: {'movement': 1.3262334910309403, 'queen': 6.313073427980955, 'unused_tiles': 0.8972861546955391}. Best is trial 0 with value: 0.6.
[I 2021-06-03 14:27:18,151] Trial 2 finished with value: 0.4 and parameters: {'movement': 6.564873868622458, 'queen': 5.7169924839844, 'unused_tiles': 6.1062035961431596}. Best is trial 0 with value: 0.6.
[I 2021-06-03 14:27:28,443] Trial 3 finished with value: 0.325 and parameters: {'movement': 8.454187852445408, 'queen': 4.155033581761077, 'unused_tiles': 0.7070096319184793}. Best is trial 0 with value: 0.6.
[I 2021-06-03 14:27:36,069] Trial 4 finished with value: 0.125 and parameters: {'movement': 4.101950242502937, 'queen': 7.998293455305162, 'unused_tiles': 4.356654356611383}. Best is trial 0 with value: 0.6.
[I 2021-06-03 14:27:44,140] Trial 5 finished with value: 0.175 and parameters: {'movement': 7.15573496123152, 'queen': 4.32150785801735, 'unused_tiles': 6.072660349609494}. Best is trial 0 with value: 0.6.
[I 2021-06-03 14:27:53,866] Trial 6 finished with value: 0.225 and parameters: {'movement': 1.3443503069792317, 'queen': 1.5071748024854248, 'unused_tiles': 3.2411718662038047}. Best is trial 0 with value: 0.6.
[I 2021-06-03 14:28:04,753] Trial 7 finished with value: 0.425 and parameters: {'movement': 9.108594429411745, 'queen': 1.516877879094084, 'unused_tiles': 3.94211925888225}. Best is trial 0 with value: 0.6.
[I 2021-06-03 14:28:18,928] Trial 8 finished with value: 0.2 and parameters: {'movement': 0.09639205397371509, 'queen': 5.188355599699976, 'unused_tiles': 8.626583488135465}. Best is trial 0 with value: 0.6.
[I 2021-06-03 14:28:28,631] Trial 9 finished with value: 0.35 and parameters: {'movement': 7.935091115774489, 'queen': 1.501834387572114, 'unused_tiles': 4.901608558378927}. Best is trial 0 with value: 0.6.
[I 2021-06-03 14:28:36,228] Trial 10 finished with value: 0.7 and parameters: {'movement': 4.162523448037564, 'queen': 0.16348359038516858, 'unused_tiles': 8.904234210459236}. Best is trial 10 with value: 0.7.
[I 2021-06-03 14:28:43,561] Trial 11 finished with value: 0.525 and parameters: {'movement': 4.5178256650928805, 'queen': 0.09627882849618641, 'unused_tiles': 9.997010092701359}. Best is trial 10 with value: 0.7.
[I 2021-06-03 14:28:51,136] Trial 12 finished with value: 0.45 and parameters: {'movement': 5.566001051861241, 'queen': 0.214028062557898, 'unused_tiles': 8.569862541336082}. Best is trial 10 with value: 0.7.
[I 2021-06-03 14:29:00,240] Trial 13 finished with value: 0.4 and parameters: {'movement': 2.706212863512989, 'queen': 3.113394183493927, 'unused_tiles': 9.690536441284383}. Best is trial 10 with value: 0.7.
[I 2021-06-03 14:29:10,878] Trial 14 finished with value: 0.375 and parameters: {'movement': 5.852007381377437, 'queen': 3.08958215031142, 'unused_tiles': 7.465780538929069}. Best is trial 10 with value: 0.7.
[I 2021-06-03 14:29:21,680] Trial 15 finished with value: 0.525 and parameters: {'movement': 3.2817566736801522, 'queen': 0.5736592431435388, 'unused_tiles': 7.451527555369798}. Best is trial 10 with value: 0.7.
[I 2021-06-03 14:29:33,748] Trial 16 finished with value: 0.375 and parameters: {'movement': 5.017445988008726, 'queen': 2.875304122721879, 'unused_tiles': 9.024349222057117}. Best is trial 10 with value: 0.7.
[I 2021-06-03 14:29:43,819] Trial 17 finished with value: 0.325 and parameters: {'movement': 9.841054236017005, 'queen': 9.827516076341109, 'unused_tiles': 6.993329357241887}. Best is trial 10 with value: 0.7.
[I 2021-06-03 14:29:51,852] Trial 18 finished with value: 0.4 and parameters: {'movement': 6.896593588581674, 'queen': 1.9241227931621037, 'unused_tiles': 2.610384285533674}. Best is trial 10 with value: 0.7.
[I 2021-06-03 14:29:59,260] Trial 19 finished with value: 0.575 and parameters: {'movement': 3.415446937525096, 'queen': 0.015855448275896133, 'unused_tiles': 9.601257374671244}. Best is trial 10 with value: 0.7.
[I 2021-06-03 14:30:06,544] Trial 20 finished with value: 0.3 and parameters: {'movement': 5.921871203924179, 'queen': 2.4034603912429193, 'unused_tiles': 8.030192154886763}. Best is trial 10 with value: 0.7.
[I 2021-06-03 14:30:16,049] Trial 21 finished with value: 0.45 and parameters: {'movement': 3.234178868567472, 'queen': 0.6396458823201584, 'unused_tiles': 9.94333814912499}. Best is trial 10 with value: 0.7.
[I 2021-06-03 14:30:23,522] Trial 22 finished with value: 0.625 and parameters: {'movement': 3.79085366820295, 'queen': 0.048106156549570456, 'unused_tiles': 9.350506404108383}. Best is trial 10 with value: 0.7.
[I 2021-06-03 14:30:32,513] Trial 23 finished with value: 0.4 and parameters: {'movement': 2.199702456667779, 'queen': 0.9278667677372152, 'unused_tiles': 6.435264385489694}. Best is trial 10 with value: 0.7.
[I 2021-06-03 14:30:43,019] Trial 24 finished with value: 0.6 and parameters: {'movement': 4.218784191699747, 'queen': 0.0717500586885434, 'unused_tiles': 8.979289838829986}. Best is trial 10 with value: 0.7.
[I 2021-06-03 14:30:51,166] Trial 25 finished with value: 0.4 and parameters: {'movement': 4.877298274230938, 'queen': 3.913393647812447, 'unused_tiles': 8.14995873303317}. Best is trial 10 with value: 0.7.
[I 2021-06-03 14:31:01,532] Trial 26 finished with value: 0.7 and parameters: {'movement': 1.908158571583041, 'queen': 0.908858359089631, 'unused_tiles': 9.265475699886892}. Best is trial 10 with value: 0.7.
[I 2021-06-03 14:31:14,744] Trial 27 finished with value: 0.225 and parameters: {'movement': 0.029224670546427856, 'queen': 6.966240180653625, 'unused_tiles': 9.325576924068582}. Best is trial 10 with value: 0.7.
[I 2021-06-03 14:31:25,314] Trial 28 finished with value: 0.475 and parameters: {'movement': 1.5992256505287643, 'queen': 1.0234052532046882, 'unused_tiles': 7.8853436917177}. Best is trial 10 with value: 0.7.
[I 2021-06-03 14:31:37,663] Trial 29 finished with value: 0.2 and parameters: {'movement': 2.188447676227265, 'queen': 2.486102558157101, 'unused_tiles': 6.7898028074306875}. Best is trial 10 with value: 0.7.
[I 2021-06-03 14:31:49,233] Trial 30 finished with value: 0.175 and parameters: {'movement': 0.6913466144188689, 'queen': 1.2578797617161213, 'unused_tiles': 5.36371638927017}. Best is trial 10 with value: 0.7.
[I 2021-06-03 14:31:59,213] Trial 31 finished with value: 0.375 and parameters: {'movement': 4.052826355209257, 'queen': 2.21346663413917, 'unused_tiles': 9.978748656955235}. Best is trial 10 with value: 0.7.
[I 2021-06-03 14:32:08,339] Trial 32 finished with value: 0.45 and parameters: {'movement': 6.313599964820663, 'queen': 0.05329933048588731, 'unused_tiles': 8.818335220698568}. Best is trial 10 with value: 0.7.
[I 2021-06-03 14:32:17,713] Trial 33 finished with value: 0.575 and parameters: {'movement': 7.6738291173402615, 'queen': 0.657753545199357, 'unused_tiles': 8.406420083385441}. Best is trial 10 with value: 0.7.
[I 2021-06-03 14:32:25,137] Trial 34 finished with value: 0.4 and parameters: {'movement': 5.175444285418088, 'queen': 2.046472940313515, 'unused_tiles': 9.37336794080675}. Best is trial 10 with value: 0.7.
[I 2021-06-03 14:32:32,272] Trial 35 finished with value: 0.4 and parameters: {'movement': 3.464109525451603, 'queen': 0.3126712527617749, 'unused_tiles': 9.18426752629883}. Best is trial 10 with value: 0.7.
[I 2021-06-03 14:32:41,435] Trial 36 finished with value: 0.05 and parameters: {'movement': 2.13481402328102, 'queen': 6.454204797596236, 'unused_tiles': 7.6672219305075044}. Best is trial 10 with value: 0.7.
[I 2021-06-03 14:32:51,863] Trial 37 finished with value: 0.175 and parameters: {'movement': 3.7935445389475078, 'queen': 3.5969136067456366, 'unused_tiles': 7.129234582060104}. Best is trial 10 with value: 0.7.
[I 2021-06-03 14:33:02,222] Trial 38 finished with value: 0.225 and parameters: {'movement': 0.8214892186162486, 'queen': 4.703339055360921, 'unused_tiles': 1.4805823432220917}. Best is trial 10 with value: 0.7.
[I 2021-06-03 14:33:11,910] Trial 39 finished with value: 0.35 and parameters: {'movement': 6.511093498508833, 'queen': 1.607418431556099, 'unused_tiles': 5.875674563722577}. Best is trial 10 with value: 0.7.
[I 2021-06-03 14:33:20,145] Trial 40 finished with value: 0.55 and parameters: {'movement': 2.593147703258115, 'queen': 1.1111419773948203, 'unused_tiles': 8.276228254576546}. Best is trial 10 with value: 0.7.
[I 2021-06-03 14:33:27,885] Trial 41 finished with value: 0.6 and parameters: {'movement': 4.357908995910319, 'queen': 0.047144310960184566, 'unused_tiles': 8.886023881902998}. Best is trial 10 with value: 0.7.
[I 2021-06-03 14:33:34,012] Trial 42 finished with value: 0.275 and parameters: {'movement': 4.6116703061757445, 'queen': 0.5341974140070354, 'unused_tiles': 9.500697151929373}. Best is trial 10 with value: 0.7.
[I 2021-06-03 14:33:43,001] Trial 43 finished with value: 0.35 and parameters: {'movement': 5.323672973879239, 'queen': 1.4553634853907327, 'unused_tiles': 0.08020671980360472}. Best is trial 10 with value: 0.7.
[I 2021-06-03 14:33:52,916] Trial 44 finished with value: 0.575 and parameters: {'movement': 4.468164503782404, 'queen': 0.8388482736924409, 'unused_tiles': 8.73293435879007}. Best is trial 10 with value: 0.7.
[I 2021-06-03 14:34:00,605] Trial 45 finished with value: 0.475 and parameters: {'movement': 4.0419792959615775, 'queen': 0.09916157188119218, 'unused_tiles': 8.93865659218047}. Best is trial 10 with value: 0.7.
[I 2021-06-03 14:34:08,039] Trial 46 finished with value: 0.65 and parameters: {'movement': 2.962361646636876, 'queen': 0.25505952962129774, 'unused_tiles': 9.970838690329876}. Best is trial 10 with value: 0.7.
[I 2021-06-03 14:34:16,147] Trial 47 finished with value: 0.125 and parameters: {'movement': 2.830959317203019, 'queen': 9.629321226338128, 'unused_tiles': 9.98447728294755}. Best is trial 10 with value: 0.7.
[I 2021-06-03 14:34:27,978] Trial 48 finished with value: 0.225 and parameters: {'movement': 1.5576129572655026, 'queen': 1.7471573432236633, 'unused_tiles': 9.68817593195534}. Best is trial 10 with value: 0.7.
[I 2021-06-03 14:34:36,823] Trial 49 finished with value: 0.45 and parameters: {'movement': 8.64051857217723, 'queen': 1.1744987101656286, 'unused_tiles': 8.465640016115394}. Best is trial 10 with value: 0.7.
[I 2021-06-03 14:34:45,758] Trial 50 finished with value: 0.275 and parameters: {'movement': 2.9031894817041617, 'queen': 2.9120271675068787, 'unused_tiles': 9.964834468484488}. Best is trial 10 with value: 0.7.
[I 2021-06-03 14:34:53,539] Trial 51 finished with value: 0.425 and parameters: {'movement': 3.693601230636763, 'queen': 0.36758369246820133, 'unused_tiles': 9.139309728012252}. Best is trial 10 with value: 0.7.
[I 2021-06-03 14:35:01,014] Trial 52 finished with value: 0.675 and parameters: {'movement': 5.589314550669937, 'queen': 0.46811142107808656, 'unused_tiles': 7.8209459123039675}. Best is trial 10 with value: 0.7.
[I 2021-06-03 14:35:08,346] Trial 53 finished with value: 0.45 and parameters: {'movement': 7.34942689204359, 'queen': 0.8256731861048721, 'unused_tiles': 7.557300120778928}. Best is trial 10 with value: 0.7.
[I 2021-06-03 14:35:16,145] Trial 54 finished with value: 0.5 and parameters: {'movement': 5.534063651893781, 'queen': 0.04243274733855884, 'unused_tiles': 8.556584555422475}. Best is trial 10 with value: 0.7.
[I 2021-06-03 14:35:24,694] Trial 55 finished with value: 0.525 and parameters: {'movement': 5.947061513655348, 'queen': 0.4683749343816346, 'unused_tiles': 9.676293364635175}. Best is trial 10 with value: 0.7.
[I 2021-06-03 14:35:32,827] Trial 56 finished with value: 0.1 and parameters: {'movement': 4.872731250819724, 'queen': 5.360013379166539, 'unused_tiles': 7.99679964772523}. Best is trial 10 with value: 0.7.
[I 2021-06-03 14:35:41,854] Trial 57 finished with value: 0.425 and parameters: {'movement': 3.0880335691161074, 'queen': 1.3509222837741603, 'unused_tiles': 4.255170734528831}. Best is trial 10 with value: 0.7.
[I 2021-06-03 14:35:49,017] Trial 58 finished with value: 0.375 and parameters: {'movement': 6.949603884053044, 'queen': 2.6584228597902104, 'unused_tiles': 9.323107951921394}. Best is trial 10 with value: 0.7.
[I 2021-06-03 14:36:02,243] Trial 59 finished with value: 0.3 and parameters: {'movement': 2.481731469148964, 'queen': 1.9764667093573927, 'unused_tiles': 8.263135143944979}. Best is trial 10 with value: 0.7.
[I 2021-06-03 14:36:11,504] Trial 60 finished with value: 0.65 and parameters: {'movement': 1.0056563805125402, 'queen': 0.44374945405879085, 'unused_tiles': 7.244921598716512}. Best is trial 10 with value: 0.7.
[I 2021-06-03 14:36:22,770] Trial 61 finished with value: 0.525 and parameters: {'movement': 0.5254549118225325, 'queen': 0.48482874298594825, 'unused_tiles': 7.202104172801505}. Best is trial 10 with value: 0.7.
[I 2021-06-03 14:36:31,432] Trial 62 finished with value: 0.575 and parameters: {'movement': 1.7899451891623093, 'queen': 0.7862360872034266, 'unused_tiles': 7.77567022328022}. Best is trial 10 with value: 0.7.
[I 2021-06-03 14:36:39,081] Trial 63 finished with value: 0.35 and parameters: {'movement': 6.19384296561725, 'queen': 1.072212447806372, 'unused_tiles': 6.713484107530952}. Best is trial 10 with value: 0.7.
[I 2021-06-03 14:36:51,053] Trial 64 finished with value: 0.3 and parameters: {'movement': 1.0313482694808809, 'queen': 1.7184475189225452, 'unused_tiles': 8.613771858114427}. Best is trial 10 with value: 0.7.
[I 2021-06-03 14:36:57,732] Trial 65 finished with value: 0.5 and parameters: {'movement': 4.377785434165459, 'queen': 0.09513745694781069, 'unused_tiles': 8.906195324241539}. Best is trial 10 with value: 0.7.
[I 2021-06-03 14:37:05,222] Trial 66 finished with value: 0.7 and parameters: {'movement': 2.0047124050497684, 'queen': 0.42510644049303337, 'unused_tiles': 6.331889724056682}. Best is trial 10 with value: 0.7.
[I 2021-06-03 14:37:12,072] Trial 67 finished with value: 0.5 and parameters: {'movement': 1.2210011589066818, 'queen': 0.41815358287857096, 'unused_tiles': 6.228346832950933}. Best is trial 10 with value: 0.7.
[I 2021-06-03 14:37:24,676] Trial 68 finished with value: 0.25 and parameters: {'movement': 0.2791672732754257, 'queen': 0.7888185795927355, 'unused_tiles': 5.7606140751637875}. Best is trial 10 with value: 0.7.
[I 2021-06-03 14:37:32,269] Trial 69 finished with value: 0.65 and parameters: {'movement': 2.0242118167770315, 'queen': 0.30734433253703464, 'unused_tiles': 5.116256224831795}. Best is trial 10 with value: 0.7.
[I 2021-06-03 14:37:41,919] Trial 70 finished with value: 0.1 and parameters: {'movement': 1.878792480565029, 'queen': 8.371740645364037, 'unused_tiles': 4.838175679959488}. Best is trial 10 with value: 0.7.
[I 2021-06-03 14:37:47,907] Trial 71 finished with value: 0.75 and parameters: {'movement': 2.2544648514552357, 'queen': 0.001573033673776092, 'unused_tiles': 5.202154033589205}. Best is trial 71 with value: 0.75.
[I 2021-06-03 14:37:54,946] Trial 72 finished with value: 0.525 and parameters: {'movement': 2.3032022674365447, 'queen': 0.3325141126633786, 'unused_tiles': 5.271650712934201}. Best is trial 71 with value: 0.75.
[I 2021-06-03 14:38:06,805] Trial 73 finished with value: 0.2 and parameters: {'movement': 1.223429218946038, 'queen': 1.340751005154238, 'unused_tiles': 4.7915593215789105}. Best is trial 71 with value: 0.75.
[I 2021-06-03 14:38:16,599] Trial 74 finished with value: 0.425 and parameters: {'movement': 1.9688679720584672, 'queen': 0.6404598268032329, 'unused_tiles': 3.6851357880737527}. Best is trial 71 with value: 0.75.
[I 2021-06-03 14:38:23,879] Trial 75 finished with value: 0.7 and parameters: {'movement': 1.5868755859602983, 'queen': 0.0033299045707875785, 'unused_tiles': 5.605203260894941}. Best is trial 71 with value: 0.75.
[I 2021-06-03 14:38:33,662] Trial 76 finished with value: 0.425 and parameters: {'movement': 1.514777779747681, 'queen': 1.0293949176342003, 'unused_tiles': 5.561159942933097}. Best is trial 71 with value: 0.75.
[I 2021-06-03 14:38:42,003] Trial 77 finished with value: 0.7 and parameters: {'movement': 2.495206493130299, 'queen': 0.0006012448847577678, 'unused_tiles': 5.2210394826768205}. Best is trial 71 with value: 0.75.
[I 2021-06-03 14:38:50,081] Trial 78 finished with value: 0.55 and parameters: {'movement': 2.5086268148814814, 'queen': 0.01760246359020845, 'unused_tiles': 4.308930460134302}. Best is trial 71 with value: 0.75.
[I 2021-06-03 14:38:57,437] Trial 79 finished with value: 0.525 and parameters: {'movement': 1.6971247270825154, 'queen': 0.7030158648042951, 'unused_tiles': 5.136763058424937}. Best is trial 71 with value: 0.75.
[I 2021-06-03 14:39:05,142] Trial 80 finished with value: 0.725 and parameters: {'movement': 2.1001453860389057, 'queen': 0.005700960591277032, 'unused_tiles': 6.297865167691878}. Best is trial 71 with value: 0.75.
[I 2021-06-03 14:39:12,691] Trial 81 finished with value: 0.5 and parameters: {'movement': 2.0528564807046523, 'queen': 0.23815865720785578, 'unused_tiles': 6.408834892876955}. Best is trial 71 with value: 0.75.
[I 2021-06-03 14:39:19,995] Trial 82 finished with value: 0.45 and parameters: {'movement': 2.2519923404279534, 'queen': 0.9443921163951464, 'unused_tiles': 4.6401007840686015}. Best is trial 71 with value: 0.75.
[I 2021-06-03 14:39:28,870] Trial 83 finished with value: 0.525 and parameters: {'movement': 1.4841881962320307, 'queen': 0.021301982761929558, 'unused_tiles': 6.094924999630914}. Best is trial 71 with value: 0.75.
[I 2021-06-03 14:39:37,081] Trial 84 finished with value: 0.525 and parameters: {'movement': 2.6076717088809165, 'queen': 0.6006028576515701, 'unused_tiles': 6.7913050437948606}. Best is trial 71 with value: 0.75.
[I 2021-06-03 14:39:44,612] Trial 85 finished with value: 0.5 and parameters: {'movement': 3.233997961236178, 'queen': 0.27576050434956, 'unused_tiles': 5.519276499077258}. Best is trial 71 with value: 0.75.
[I 2021-06-03 14:39:51,231] Trial 86 finished with value: 0.5 and parameters: {'movement': 3.053060887236463, 'queen': 0.032046064895154644, 'unused_tiles': 5.827626525846364}. Best is trial 71 with value: 0.75.
[I 2021-06-03 14:39:57,773] Trial 87 finished with value: 0.525 and parameters: {'movement': 3.509646112724056, 'queen': 1.2282644489719226, 'unused_tiles': 6.527848013004465}. Best is trial 71 with value: 0.75.
[I 2021-06-03 14:40:05,723] Trial 88 finished with value: 0.5 and parameters: {'movement': 2.370202254693938, 'queen': 0.9243379543768167, 'unused_tiles': 3.8652622586255143}. Best is trial 71 with value: 0.75.
[I 2021-06-03 14:40:15,391] Trial 89 finished with value: 0.375 and parameters: {'movement': 0.9024718022425298, 'queen': 0.6358699773657627, 'unused_tiles': 7.070973972647493}. Best is trial 71 with value: 0.75.
[I 2021-06-03 14:40:23,103] Trial 90 finished with value: 0.15 and parameters: {'movement': 0.4314881527577423, 'queen': 1.579601357495167, 'unused_tiles': 7.37655162436641}. Best is trial 71 with value: 0.75.
[I 2021-06-03 14:40:31,300] Trial 91 finished with value: 0.55 and parameters: {'movement': 2.8629879498722657, 'queen': 0.29131129958558766, 'unused_tiles': 5.167709297823194}. Best is trial 71 with value: 0.75.
[I 2021-06-03 14:40:39,139] Trial 92 finished with value: 0.475 and parameters: {'movement': 1.8930885850976327, 'queen': 0.2844629777776388, 'unused_tiles': 4.549876854593206}. Best is trial 71 with value: 0.75.
[I 2021-06-03 14:40:47,443] Trial 93 finished with value: 0.7 and parameters: {'movement': 2.7674752178018034, 'queen': 0.02131162209052362, 'unused_tiles': 5.991889826791184}. Best is trial 71 with value: 0.75.
[I 2021-06-03 14:40:57,319] Trial 94 finished with value: 0.55 and parameters: {'movement': 2.7544256378327274, 'queen': 0.5114645537188579, 'unused_tiles': 5.53634620035003}. Best is trial 71 with value: 0.75.
[I 2021-06-03 14:41:05,554] Trial 95 finished with value: 0.5 and parameters: {'movement': 1.3539326285514348, 'queen': 0.0009807601020506453, 'unused_tiles': 6.2905538556683664}. Best is trial 71 with value: 0.75.
[I 2021-06-03 14:41:15,175] Trial 96 finished with value: 0.525 and parameters: {'movement': 3.059020856283574, 'queen': 0.1504824546723306, 'unused_tiles': 5.786829335742334}. Best is trial 71 with value: 0.75.
[I 2021-06-03 14:41:21,820] Trial 97 finished with value: 0.625 and parameters: {'movement': 2.061237997780237, 'queen': 0.8781700928457338, 'unused_tiles': 5.006985026556407}. Best is trial 71 with value: 0.75.
[I 2021-06-03 14:41:28,101] Trial 98 finished with value: 0.4 and parameters: {'movement': 2.3708673881588918, 'queen': 1.161996729644701, 'unused_tiles': 5.526815374386066}. Best is trial 71 with value: 0.75.
[I 2021-06-03 14:41:36,218] Trial 99 finished with value: 0.025 and parameters: {'movement': 1.0858693861129058, 'queen': 3.47749256228966, 'unused_tiles': 6.627270333814565}. Best is trial 71 with value: 0.75.
"""
    trial = []
    value = []
    pm = []
    pq = []
    pu = []
    for m in re.finditer("Trial ([\d]+) finished with value: (\d\.[\d]+) and parameters: \{'movement': (\d\.[\d]+), 'queen': (\d\.[\d]+), 'unused_tiles': (\d\.[\d]+)\}", data):
        trial.append(m.group(1))
        value.append(m.group(2))
        pm.append(m.group(3))
        pq.append(m.group(4))
        pu.append(m.group(5))

    trial = [int(t) for t in trial]
    value = [float(v) for v in value]
    pm = [float(v) for v in pm]
    pq = [float(v) for v in pq]
    pu = [float(v) for v in pu]

    pm = np.convolve(pm, [.25, .5, .25], "same")
    pq = np.convolve(pq, [.25, .5, .25], "same")
    pu = np.convolve(pu, [.25, .5, .25], "same")

    # fig, ax1 = plt.subplots()
    plt.plot(trial, value, c="black")
    plt.xlabel("Trial")
    plt.ylabel("Win factor")
    plt.ylim((0, 1))
    plt.show()

    # ax2 = ax1.twinx()
    plt.plot(trial, pm)
    plt.plot(trial, pq)
    plt.plot(trial, pu)
    plt.xlabel("Trial")
    plt.ylabel("Parameter Value")
    plt.ylim((0, 10))
    plt.legend(["Movement", "Queen", "Unused Tiles"])
    plt.show()


if __name__ == "__main__":

    # plot()
    main()
