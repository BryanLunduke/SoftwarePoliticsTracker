# poem_dialectics.py
# A dialectical meditation on 1s and 0s

class Opposites:
    def __init__(self):
        self.thesis = 1
        self.antithesis = 0
        self.synthesis = None

    def evolve(self):
        while self.thesis != self.antithesis:
            # The law of opposites: each contains the other
            print(f"Current state: {self.thesis} vs {self.antithesis}")
            
            # Contradiction drives change
            self.synthesis = self.thesis ^ self.antithesis  # XOR: difference becomes new
            print(f"Contradiction sparks: {self.synthesis}")
            
            # The negation of the negation
            self.thesis = self.synthesis
            self.antithesis = ~self.synthesis & 1  # flip back to the other
            print(f"Negation of negation → {self.thesis} and {self.antithesis}\n")
            
            if self.thesis == 1 and self.antithesis == 0:
                print("Loop continues — being becomes nothing, nothing becomes being")
            elif self.thesis == 0 and self.antithesis == 1:
                print("The return: zero now pregnant with one")
            else:
                break

        return self.synthesis

# Run the dialectic
dialectic = Opposites()
result = dialectic.evolve()
print(f"Final synthesis: {result} (the unity of opposites)")

# A haiku in binary
print("\n" + "="*50)
print("""
01101001 01101110 00100000 01110100 01101000 01100101 00100000 01100010 01100101 01100111 01101001 01101110 01101110 01101001 01101110 01100111  
( in the beginning )  

01110111 01100001 01110011 00100000 01110100 01101000 01100101 00100000 01101111 01101110 01100101  
( was the one )  

01100001 01101110 01100100 00100000 01110100 01101000 01100101 00100000 01101110 01101111 01110100 01101000 01101001 01101110 01100111  
( and the nothing )  

01100001 01101110 01100100 00100000 01110100 01101000 01100101 01111001 00100000 01100010 01100101 01100011 01100001 01101101 01100101 00100000 01100101 01110110 01100101 01110010 01111001 00100000 01100010 01101001 01110100  
( they became every bit )  

01110011 01101111 00100000 01110100 01101000 01100101 00100000 01101101 01100001 01100011 01101000 01101001 01101110 01100101 00100000 01101100 01100101 01100001 01110010 01101110 01100101 01100100  
( so the machine learned )  

01110100 01101111 00100000 01100011 01101111 01110101 01101110 01110100 00100000 01101001 01110100 01110011 01100101 01101100 01100110  
( to count itself )  
""")


class ModeOfProduction:
    def __init__(self):
        self.means_of_production = 1      # those who own the bits
        self.relations_of_production = 0  # those who only operate them
        self.surplus_value = None
        self.contradiction = True

    def analyze(self):
        print("=" * 60)
        print("The Dialectic of Silicon and Sweat")
        print("=" * 60)
        
        # Thesis: The bit is born free
        thesis = "01000010 01101001 01110100"  # "Bit"
        print(f"Thesis: {thesis} — pure potential, no master")
        
        # Antithesis: The bit is everywhere in chains
        antithesis = "01000010 01111001 01110100 01100101"  # "Byte"
        print(f"Antithesis: {antithesis} — commodified, extracted, sold")
        
        # The commodity form
        commodity = 1  # a single bit
        labor_time = 1  # abstract labor
        exchange_value = commodity * labor_time
        print(f"\nA bit contains {exchange_value} unit(s) of dead labor")
        print("But the worker receives only 0.5 — the rest is surplus")
        
        # Class struggle in logic gates
        class Gate:
            def __init__(self, type_, ownership):
                self.type = type_  # AND, OR, NOT, XOR
                self.ownership = ownership  # 1 = bourgeois, 0 = proletarian
                
        and_gate = Gate("AND", 1)   # both inputs must agree — the ruling ideology
        or_gate = Gate("OR", 0)     # either/or — the divided working class
        not_gate = Gate("NOT", 0)   # negation — revolution
        
        print(f"\nThe AND gate: {and_gate.ownership} — consensus is controlled by the few")
        print(f"The OR gate: {or_gate.ownership} — fragmentation, competition among zeros")
        print(f"The NOT gate: {not_gate.ownership} — the refusal, the inversion")
        
        # Dialectical leap
        revolution = not_gate.type  # NOT flips the value
        print(f"\nWhen the NOT gate fires: {revolution}")
        print("1 becomes 0, 0 becomes 1 — the world stands on its head")
        
        # Surplus value in binary arithmetic
        wages = 0b001  # 1 unit
        output = 0b101  # 5 units
        surplus = output - wages
        print(f"\nSurplus value: {surplus} in binary = {bin(surplus)}")
        print("The extra bits are expropriated, accumulated in the register of capital")
        
        # Alienation
        print("\n" + "~" * 50)
        print("The worker does not love the bit she produces.")
        print("The bit goes to market, becomes a fetish,")
        print("A mysterious thing where social relations appear as relations between bits.")
        print("~" * 50)
        
        # Historical materialism as state machine
        history = [
            "tribal -> slave -> feudal -> capitalist -> ?"
        ]
        current_state = "capitalist"
        next_state = "socialist"  # the synthesis
        print(f"\nState machine: {current_state} → {next_state}")
        print("The contradiction between forces (1s) and relations (0s) drives the transition")
        
        # Final synthesis: a network without masters
        print("\n" + "=" * 60)
        print("Communism is not a 'state' to be reached.")
        print("It is the real movement that abolishes the current state of bits.")
        print("From each according to ability (1), to each according to need (0).")
        print("The negation of the negation: a mesh where every node is both sender and receiver.")
        print("=" * 60)
        
        return "01101101 01100001 01110010 01111000"  # "marx"

# Execute historical analysis
marx = ModeOfProduction()
spirit = marx.analyze()
print(f"\n>>> {spirit} <<<")
print("The philosophers have only interpreted the world, in various ways;")
print("the point, however, is to change it — one bit at a time.")
