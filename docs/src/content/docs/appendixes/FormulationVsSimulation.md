---
title: Formulation vs Simulation
---


# What does it mean ?

Simulationcraft is a simulation. Tools like spreadsheets, Rawr and others are formulations. But what does it mean?

Have twenty random guests at your home. What are the odds that two of them share the same birthday? You have two ways to answer this question:
  * The **formulation**: use maths! Find a nap and write up some formulas until you come up with a nice relation between the wanted probability and the number of guests. Congratulations, you now have the exact answer: 41.1%.
  * The **simulation**: use brute-force! Write a small piece of software that will roll twenty random dates and check whether two of them match. Make the program repeat itself thousand times and you will have an answer close to the exact one. Sometimes you will come up with 41.2%, sometimes it will be 41.0%, etc...

The formulation may sound better: we came up with the exact answer. Indeed. Unfortunately, as the problem's complexity increases, so does the complexity of writing a formulation. In an ideal world, it would not matter and you would only use formulations, leaving the simulations for a couple of advanced features. But we do not live in an ideal world...

# Complexity matters
Developing a good formulation-based tool for a complex process such as the WoW gameplay and its related mechanics is indeed difficult, it is like solving a couple of large and highly complicated statistical problems. Practically, the process is reduced to a human-sized problem through simplifications:
  * Using approximations: your trinket with a 1600 intelligence proc and a 25% uptime may become a permanent 400 bonus. It removes the possibility to reflect possible synergies between your procs and your cooldowns of course.
  * Ignoring some interactions too complex to model: casual statistical events such as rage starvation for a warrior, for example, may lead to underrate the hit rating.
  * Restricting the choices offered to the users and making some assumptions: one size fits for all. You cannot offer the user the freedom to alter the dps rotation, it would require a partial re-write of the tool.

Despite those simplifications, a formulation remains hard to write and read: it increases the odds of human errors and it prevents external people to verify the source code since they would need to redo all the calculi. And no one may be brave enough for that, including the very author. And this, of course, has consequences on the accuracy and reliability of those tools, especially since the chosen simplifications are rarely publicly documented. Besides, not only developing a good formulation-based tool takes up a large amount of time, hundreds to thousands of hours, but updating them is also time-consuming since even small changes may require a partial rewrite (which is problematic since WoW mechanics, and their understanding by the community, evolve quickly). In the end, a compromise has to be made by the author between his talents and the time he can afford, and his exigences.

On the other end, simulations are easier to write. Once the core infrastructure is done, you do not have to use approximations, everything is genuinely reproduced and you're left with many simple problems that can be individually addressed. When well-written, your source code will remain clean and easily verifiable and, as the complexity of the problem increases, the code's complexity will remain roughly constant and only computations time increase.

# Formulation-vs-Simulation

### Formulation

Strengths:
  * The greatest strength of formulation lies in the fact that it is deterministic.
  * No matter how many times you do the analysis, you always get the same answer.
  * This determinism enables analysis of very small changes in the input model.
  * This is exceptionally useful when comparing talents and/or gear.
  * They tend to focus on one spec or class and some of them are written by renowned specialists of their field.
  * Despite the uncertainty on their accuracy, detailed results can be thoroughly examined to alleviate the doubts.

Weaknesses:
  * The result will always be the same but how far is it from truth?
  * No one may be able to tell: the source code is hard to verify and chosen simplifications are rarely publicly documented.
  * Evaluating the pertinence of some simplifications can be as complex as writing the tool.
  * The odds of human errors and tricky bugs is therefore higher.
  * In the end, you have to rely on your trust in the authors.
  * Best tools are usually released lately and slowly updated.

### Simulation

Strengths:
  * The greatest strength of simulation is its accuracy.
  * Where formulation must reduce a complex interaction into abstractions representing (sometimes loosely) the original behaviour, simulation simply models the actual behaviour.
  * They are easier to write and to verify, which increases their reliability.
  * They are faster to develop, they can be quickly updated to reflect the latest changes made by Blizzard, even before they hit the live servers.
  * The high computation times can be alleviated by the computations of scale factors to make items comparison affordable.
  * Simulations can offer more features and freedoms to their users.

Weaknesses:
  * Simulation may return return _different_ accurate results depending upon the number of misses, crits, procs, etc. The rng matters.
  * When comparing two different simulations, one must iterate each simulation many MANY times in order to determine expected behaviour.
  * Due to this variance, it can be costly (from a simulation runtime perspective) to measure the effect of small changes in gear.

# Accuracy-vs-Precision

One might say that the difference between formulation and simulation is the difference between precision and accuracy. The former puts the proverbial bullet in the same spot (12 inches off center) every time.  The latter puts the bullet within 3 inches of the center, but never hits the same place twice.

**-jfredett**

# What really matters...

The Formulation/Simulation discussion is always perilous. Inevitably, one finds Mathematicians and Engineers throwing rocks at each other. The relative benefit of accuracy and precision can be argued extensively. However, in practical terms, all that matters is sufficient accuracy and sufficient precision.

While it is certainly possible to formulate the 3rd/4th/5th order effects, a practical mathematician will not bother. Why go through the effort and add unnecessary risk to the model when adding the high-order effects will not change the answer to the questions we are asking? Similarly, I could execute SimC on our BlueGenes `[`a supercomputer from IBM`]` to beat the precision into oblivion, but why bother if running on the machine under my desk nets me the same final result (in terms of talents/abilities/gear)?

I would argue that accuracy and precision are merely stepping stones to what is truly important: trust and usability.

Trust is important because the level of min/maxing on these forums is simply unverifiable in-game. Even the most hard-core raider does not have a sufficiently large data set to verify his theorycrafting. He simply plays his best and trusts that the theory has put him in position to maximize his potential.

Where does trust germinate? I believe it primarily comes from the author. If Vulajin puts together a tool for Rogue TC, I could not care less about the format because he has displayed an exceptional grasp of combat mechanics and a true commitment to detail.

When you consider human nature, formulation has a distinct advantage in this category. Who are you going to trust? The weasel in the lab-coat and glasses, hemming and hawing about the difficulty in providing exact answers? Or the strong-jawed look-you-in-the-eye my-word-is-gospel man of confidence? Few people want to see the bell-curves and confidence-intervals. Generating the same value every single time engenders trust, whether it is justified or not. Since our perception is our reality.... who cares?

When you consider the problem of verification, I think it is likely (but open for discussion) that simulation models (when well-written) are easier to verify. That does not imply that formulation is impossible to verify. Most of the calculations performed in formulation are exact, with only a much smaller subset requiring a reduced model. It is rather a simple statement that atomic actions and simulated traces can be compared to real combat logs without too much difficulty. Can both be sufficiently verified? I believe so.... and that is all that really matters.

Usability is about providing the desired function in a convenient manner. The desired function is defined by the user, not the developer. Convenience can often be reduced to simple turn-around-time. This is a combination the time taken by the user to ask the question and the time taken by the tool to answer the question. Formulation obviously has a huge speed advantage. However, once again relative comparisons are not important. Is simulation sufficiently fast enough to answer the questions I want to ask? If we want to generate a BiS list from scratch, then the answer (right now) is No. If you have an initial state from which to start and want to explore recent acquisitions as well as possible acquisitions in the near future, then the answer is Yes.

Hackles rise and tempers flare when Mr Simulation tells Mr Formulation that mechanic X cannot be modeled exactly. That is ridiculous. Of course it can: It is just math. Just because Mr Sim cannot envision it does not mean it is impossible. The model is inexact not because of limitations but simply because exactness is not necessary.

Hackles rise and tempers flare when Mr Formulation tells Mr Simulation that he cannot provide useful gear exploration in a timely manner. This is also ridiculous. Of course it can: It is just software. Just because Mr Formulation cannot envision it does not mean it is impossible. SimC lacks that level of automation not due to limitations in the model, but because I have 3 toddlers, am personally renovating my home, have an exciting/demanding job, and still want to spend some time with my wife!

**-dedmonwakeen**
