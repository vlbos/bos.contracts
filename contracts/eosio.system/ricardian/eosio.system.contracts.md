<h1 class="contract">
   bidname
</h1>
---
spec-version: 0.0
title: Bid on premium account name
summary: The {{ bidname }} action places a bid on a premium account name, in the knowledge that the high bid will purchase the name.
icon: https://boscore.io/icon_256.png#b264b855c6d3335e5ee213443f679fb87c3633de8bc31cf66a766daac6dc6d7c
---

As an authorized party I {{ signer }} wish to bid on behalf of {{ bidder }} the amount of {{ bid }} toward purchase of the account name {{ newname }}.

<h1 class="contract">
   buyram
</h1>
---
spec-version: 0.0
title: Buy RAM
summary: This action will attempt to reserve about {{quant}} worth of RAM on behalf of {{receiver}}.
icon: https://boscore.io/icon_256.png#b264b855c6d3335e5ee213443f679fb87c3633de8bc31cf66a766daac6dc6d7c
---

{{buyer}} authorizes this contract to transfer {{quant}} to buy RAM based upon the current price as determined by the market maker algorithm.

{{buyer}} accepts that a 0.5% fee will be charged on the amount spent and that the actual RAM received may be slightly less than expected due to the approximations necessary to enable this service. {{buyer}} accepts that a 0.5% fee will be charged if and when they sell the RAM received. {{buyer}} accepts that rounding errors resulting from limits of computational precision may result in less RAM being allocated. {{buyer}} acknowledges that the supply of RAM may be increased at any time up to the limits of off-the-shelf computer equipment and that this may result in RAM selling for less than purchase price. {{buyer}} acknowledges that the price of RAM may increase or decrease over time according to supply and demand. {{buyer}} acknowledges that RAM is non-transferrable. {{buyer}} acknowledges RAM currently in use by their account cannot be sold until it is freed and that freeing RAM may be subject to terms of other contracts.

<h1 class="contract">
   buyrambytes
</h1>
---
spec-version: 0.0
title: Buy RAM Bytes
summary: This action will attempt to reserve about {{bytes}} bytes of RAM on behalf of {{receiver}}.
icon: https://boscore.io/icon_256.png#b264b855c6d3335e5ee213443f679fb87c3633de8bc31cf66a766daac6dc6d7c
---

{{buyer}} authorizes this contract to transfer sufficient EOS tokens to buy the RAM based upon the current price as determined by the market maker algorithm.

{{buyer}} accepts that a 0.5% fee will be charged on the EOS spent and that the actual RAM received may be slightly less than requested due to the approximations necessary to enable this service. {{buyer}} accepts that a 0.5% fee will be charged if and when they sell the RAM received. {{buyer}} accepts that rounding errors resulting from limits of computational precision may result in less RAM being allocated. {{buyer}} acknowledges that the supply of RAM may be increased at any time up to the limits of off-the-shelf computer equipment and that this may result in RAM selling for less than purchase price. {{buyer}} acknowledges that the price of RAM may increase or decrease over time according to supply and demand. {{buyer}} acknowledges that RAM is non-transferable. {{buyer}} acknowledges RAM currently in use by their account cannot be sold until it is freed and that freeing RAM may be subject to terms of other contracts.

<h1 class="contract">
   canceldelay
</h1>
---
spec-version: 0.0
title: Cancel delayed transaction
summary: The {{ canceldelay }} action cancels an existing delayed transaction.
icon: https://boscore.io/icon_256.png#b264b855c6d3335e5ee213443f679fb87c3633de8bc31cf66a766daac6dc6d7c
---

As an authorized party I {{ signer }} wish to invoke the authority of {{ canceling_auth }} to cancel the transaction with ID {{ trx_id }}.

<h1 class="contract">
   claimrewards
</h1>
---
spec-version: 0.0
title: Claim rewards
summary: The {{ claimrewards }} action allows a block producer (active or standby) to claim the system rewards due them for producing blocks and receiving votes.
icon: https://boscore.io/icon_256.png#b264b855c6d3335e5ee213443f679fb87c3633de8bc31cf66a766daac6dc6d7c
---

As an authorized party I {{ signer }} wish to have the rewards earned by {{ owner }} deposited into the {{ owner }} account.

<h1 class="contract">
   delegatebw
</h1>
---
spec-version: 0.0
title: Delegate Bandwidth
summary: The intent of the {{ delegatebw }} action is to stake tokens for bandwidth and/or CPU and optionally transfer ownership.
icon: https://boscore.io/icon_256.png#b264b855c6d3335e5ee213443f679fb87c3633de8bc31cf66a766daac6dc6d7c
---

As an authorized party I {{ signer }} wish to stake {{ stake_cpu_quantity }} for CPU and {{ stake_net_quantity }} for bandwidth from the liquid tokens of {{ from }} for the use of delegatee {{ to }}.

<h1 class="contract">
   newaccount
</h1>
---
spec-version: 0.0
title: Create a new account
summary: The {{ newaccount }} action creates a new account.
icon: https://boscore.io/icon_256.png#b264b855c6d3335e5ee213443f679fb87c3633de8bc31cf66a766daac6dc6d7c
---

As an authorized party I {{ signer }} wish to exercise the authority of {{ creator }} to create a new account on this system named {{ name }} such that the new account's owner public key shall be {{ owner }} and the active public key shall be {{ active }}.

<h1 class="contract">
   refund
</h1>
---
spec-version: 0.0
title: Refund unstaked tokens
summary: The intent of the {{ refund }} action is to return previously unstaked tokens to an account after the unstaking period has elapsed.
icon: https://boscore.io/icon_256.png#b264b855c6d3335e5ee213443f679fb87c3633de8bc31cf66a766daac6dc6d7c
---

As an authorized party I {{ signer }} wish to have the unstaked tokens of {{ owner }} returned.

<h1 class="contract">
   sellram
</h1>
---
spec-version: 0.0
title: Sell Ram
summary: The {{ sellram }} action sells unused RAM for tokens.
icon: https://boscore.io/icon_256.png#b264b855c6d3335e5ee213443f679fb87c3633de8bc31cf66a766daac6dc6d7c
---
As an authorized party I {{ signer }} wish to sell {{ bytes }} of unused RAM from account {{ account }}.

<h1 class="contract">
   setprods
</h1>
---
spec-version: 0.0
title: Set new producer schedule
summary: The {{ setprods }} action creates a new schedule of active producers, who will produce blocks in the order given.
icon: https://boscore.io/icon_256.png#b264b855c6d3335e5ee213443f679fb87c3633de8bc31cf66a766daac6dc6d7c
---
THIS IS A SYSTEM COMMAND NOT AVAILABLE FOR DIRECT ACCESS BY USERS.

As an authorized party I {{ signer }} wish to set the rotation of producers to be {{ schedule }}.

<h1 class="contract">
   undelegatebw
</h1>
---
spec-version: 0.0
title: Undelegate bandwidth
summary: The intent of the {{ undelegatebw }} action is to unstake tokens from CPU and/or bandwidth.
icon: https://boscore.io/icon_256.png#b264b855c6d3335e5ee213443f679fb87c3633de8bc31cf66a766daac6dc6d7c
---
As an authorized party I {{ signer }} wish to unstake {{ unstake_cpu_quantity }} from CPU and {{ unstake_net_quantity }} from bandwidth from the tokens owned by {{ from }} previously delegated for the use of delegatee {{ to }}.

<h1 class="contract">
   regproducer
</h1>
---
spec-version: 0.0
title: register as Block Producer  
summary: The {{ regprod }} action registers a new block producer candidate.
icon: https://boscore.io/icon_256.png#b264b855c6d3335e5ee213443f679fb87c3633de8bc31cf66a766daac6dc6d7c
---
As an authorized party I {{ signer }} wish to register the block producer candidate {{ producer }}.

<h1 class="contract">
   unregprod
</h1>
---
spec-version: 0.0
title: Unregister as Block Producer  
summary: The {{ unregprod }} action unregisters a previously registered block producer candidate.
icon: https://boscore.io/icon_256.png#b264b855c6d3335e5ee213443f679fb87c3633de8bc31cf66a766daac6dc6d7c
---
As an authorized party I {{ signer }} wish to unregister the block producer candidate {{ producer }}, rendering that candidate no longer able to receive votes.

<h1 class="contract">
   voteproducer
</h1>
---
spec-version: 0.0
title: Vote for Block Producer(s)  
summary: The intent of the {{ voteproducer }} action is to cast a valid vote for up to 30 BP candidates.
icon: https://boscore.io/icon_256.png#b264b855c6d3335e5ee213443f679fb87c3633de8bc31cf66a766daac6dc6d7c
---

As an authorized party I {{ signer }} wish to vote on behalf of {{ voter }} in favor of the block producer candidates {{ producers }} with a voting weight equal to all tokens currently owned by {{ voter }} and staked for CPU or bandwidth.

<h1 class="contract">
   regproxy
</h1>
---
spec-version: 0.0
title: Register account as a proxy (for voting)  
summary: The intent of the {{ regproxy }} action is to register an account as a proxy for voting.
icon: https://boscore.io/icon_256.png#b264b855c6d3335e5ee213443f679fb87c3633de8bc31cf66a766daac6dc6d7c
---

As an authorized party I {{ signer }} wish to register as a {{ proxy }} account to vote on block producer candidates {{ producers }} with a voting weight equal to all tokens currently owned by {{ signer }} and tokens voted through registered {{ proxy }} by others.

<h1 class="contract">
   updateauth
</h1>
---
spec-version: 0.0
title: Update authorization of account
summary: The intent of {{ updateauth }} action is to update the authorization of an account.
icon: https://boscore.io/icon_256.png#b264b855c6d3335e5ee213443f679fb87c3633de8bc31cf66a766daac6dc6d7c
---

As an authorized party I {{ signer }} wish to update the {{ authorization }} and {{ weight }} on {{ account }}.

With `updateauth` you can update the {{ authorization }} on stated {{ account }}.

<h1 class="contract">
   deleteauth
</h1>
---
spec-version: 0.0
title: delete authorization of account
summary: The intent of {{ deleteauth }} action is to delete the authentication(s) of an account.
icon: https://boscore.io/icon_256.png#b264b855c6d3335e5ee213443f679fb87c3633de8bc31cf66a766daac6dc6d7c
---

As an authorized party I {{ signer }} wish to delete the stated {{ authorization }} on {{ account }}.

With `deleteauth` you can delete the {{ authorization }} on stated {{ account }}.

<h1 class="contract">
   linkauth
</h1>
---
spec-version: 0.0
title: link authorization with account, code, type or requirement.
summary: The intent of {{ linkauth }} action is to authorize a set permission.
icon: https://boscore.io/icon_256.png#b264b855c6d3335e5ee213443f679fb87c3633de8bc31cf66a766daac6dc6d7c
---

As an authorized party I {{ signer }} wish to link the {{ authorization }} on {{ account }} with stated {{ permission }}.

The `linkauth` action is used to set an authorization between a stated permission and the provided key or account.

<h1 class="contract">
   unlinkauth
</h1>
---
spec-version: 0.0
title: unlink authorization with account, code, type or requirement.
summary: The intent of {{ unlinkauth }} action is to remove the authorization of a permission.
icon: https://boscore.io/icon_256.png#b264b855c6d3335e5ee213443f679fb87c3633de8bc31cf66a766daac6dc6d7c
---

As an authorized party I {{ signer }} wish to remove the linked {{ authorization }} on {{ account }} with stated {{ permission }}.

The `unlinkauth` action is used to remove an linked authorization between a stated permission and the provided key or account.

<h1 class="contract">
   setabi
</h1>
---
spec-version: 0.0
title: Set ABI
summary: Set ABI to account.
icon: https://boscore.io/icon_256.png#b264b855c6d3335e5ee213443f679fb87c3633de8bc31cf66a766daac6dc6d7c
---

As an authorized party I {{ signer }} wish to set ABI to {{ account }}.

<h1 class="contract">
   buyrex
</h1>
---
spec-version: 0.0
title: Buy REX
summary:
  The `buyrex` action allows an account to buy REX in exchange for tokens taken
  out of the user's REX fund.
icon:
  https://boscore.io/icon_256.png#b264b855c6d3335e5ee213443f679fb87c3633de8bc31cf66a766daac6dc6d7c
---

As an authorized party I {{ signer }} wish to buy REX on the account {{ from }} with the amount {{ amount }} EOS. ￼I am aware of, and have fulfilled, all voting requirements needed to participate in the REX marketplace. ￼￼

<h1 class="contract">
   closerex
</h1>
---

spec-version: 0.0
title: Close REX
summary: The `closerex` action allows an account to delete unused REX-related database entries and frees occupied RAM associated with its storage.
icon: https://boscore.io/icon_256.png#b264b855c6d3335e5ee213443f679fb87c3633de8bc31cf66a766daac6dc6d7c
---

As an authorized party, I {{ signer }}, wish to delete all unused REX-related database entries from the account {{ owner }}.

I will not be able to successfully call `closerex` unless all checks for CPU loans, NET loans or refunds pending refunds are still processing on the account {{ owner }}.

<h1 class="contract">
   cnclrexorder
</h1>
---

spec-version: 0.0
title: Cancel REX order
summary: The `cnclrexorder` action cancels a queued REX sell order if one exists for an account.

icon: https://boscore.io/icon_256.png#b264b855c6d3335e5ee213443f679fb87c3633de8bc31cf66a766daac6dc6d7c
---

As an authorized party I, {{ signer }}, wish to cancel any unfilled and queued REX sell orders that exist for the account {{ owner }}.

<h1 class="contract">
   consolidate
</h1>
---

spec-version: 0.0
title: Consolidate REX
summary: The `consolidate` action will consolidate all REX maturity buckets for an account into one that matures 4 days from 00:00 UTC.

icon: https://boscore.io/icon_256.png#b264b855c6d3335e5ee213443f679fb87c3633de8bc31cf66a766daac6dc6d7c
---

As an authorized party I, {{ signer }}, wish to consolidate any open REX maturity buckets for the account {{ owner }} into one that matures 4 days from the following 00:00 UTC.

<h1 class="contract">
   defcpuloan
</h1>
---

spec-version: 0.0
title: Withdraw CPU loan
summary: The `defcpuloan` action allows an account to withdraw tokens from the fund of a specific CPU loan and adds them to REX fund.

icon: https://boscore.io/icon_256.png#b264b855c6d3335e5ee213443f679fb87c3633de8bc31cf66a766daac6dc6d7c
---

As an authorized party I, {{ signer }}, wish to withdraw from the CPU loan fund identified by loan number {{ loan_num }} on the account {{ from }} in the amount of {{ amount }} and have those tokens allocated to the REX fund of {{ from }}.

<h1 class="contract">
   defnetloan
</h1>
---

spec-version: 0.0
title: Withdraw NET loan
summary: The `defnetloan` action allows an account to withdraw tokens from the fund of a specific Network loan and adds them to REX fund.

icon: https://boscore.io/icon_256.png#b264b855c6d3335e5ee213443f679fb87c3633de8bc31cf66a766daac6dc6d7c
---

As an authorized party I, {{ signer }}, wish to withdraw from the Network loan fund identified by loan number {{ loan_num }} on the account {{ from }} in the amount of {{ amount }} and have those tokens allocated to the REX fund of {{ from }}.

<h1 class="contract">
   deposit
</h1>
---

spec-version: 0.0
title: Deposit EOS into REX
summary: The `deposit` action allows an account to deposit EOS tokens into REX fund by transfering from their liquid token balance.

icon: https://boscore.io/icon_256.png#b264b855c6d3335e5ee213443f679fb87c3633de8bc31cf66a766daac6dc6d7c
---

As an authorized party I, {{ signer }}, wish to deposit {{ amount }} EOS tokens into the REX fund of the account {{ owner }} from the liquid token balance of {{ owner }}.

<h1 class="contract">
   fundcpuloan
</h1>
---

spec-version: 0.0
title: Fund CPU Loan
summary: The `fundcpuloan` action allows an account to transfer tokens from its REX fund to the fund of a specific CPU loan in order for those tokens to be used for loan renewal at the loan's expiry.

icon: https://boscore.io/icon_256.png#b264b855c6d3335e5ee213443f679fb87c3633de8bc31cf66a766daac6dc6d7c
---

As an authorized party I, {{ signer }}, wish to transfer the amount of {{ payment }} tokens into the CPU loan fund of the loan identified by loan number {{ loan_num }} from the account {{ from }} to be used for loan renewal at the expiry of {{ loan_num }}.

<h1 class="contract">
   fundnetloan
</h1>
---

spec-version: 0.0
title: Fund NET Loan
summary: The `fundnetloan` action allows an account to transfer tokens from its REX fund to the fund of a specific Network loan in order for those tokens to be used for loan renewal at the loan's expiry.

icon: https://boscore.io/icon_256.png#b264b855c6d3335e5ee213443f679fb87c3633de8bc31cf66a766daac6dc6d7c
---

As an authorized party I, {{ signer }}, wish to transfer the amount of {{ payment }} tokens into the Network loan fund of the loan identified by loan number {{ loan_num }} from the account {{ from }} to be used for loan renewal at the expiry of {{ loan_num }}.

<h1 class="contract">
   mvfrsavings
</h1>
---

spec-version: 0.0
title: Move REX from savings
summary: The `mvfrsavings` action allows an account to move REX tokens from its savings bucket to a bucket with a maturity date that is 4 days after 00:00 UTC.

icon: https://boscore.io/icon_256.png#b264b855c6d3335e5ee213443f679fb87c3633de8bc31cf66a766daac6dc6d7c
---

As an authorized party I, {{ signer }}, wish to move {{ rex }} tokens from the savings bucket of the account {{ owner }}. Those tokens shall become available to {{ owner }} 4 days from 00:00 UTC.

<h1 class="contract">
   mvtosavings
</h1>
---

spec-version: 0.0
title: Move REX to savings
summary: The `mvtosavings` action allows an account to move REX tokens into a savings bucket.

icon: https://boscore.io/icon_256.png#b264b855c6d3335e5ee213443f679fb87c3633de8bc31cf66a766daac6dc6d7c
---

As an authorized party I, {{ signer }}, wish to move {{ rex }} tokens to a savings bucket associated to the account {{ owner }}. I acknowledge that those tokens will then be subject to any maturity restrictions described in the `mvfrsavings` action.

<h1 class="contract">
   updaterex
</h1>
---

spec-version: 0.0
title: Update REX
summary: The `updaterex` action updates the vote stake of the account.

icon: https://boscore.io/icon_256.png#b264b855c6d3335e5ee213443f679fb87c3633de8bc31cf66a766daac6dc6d7c
---

As an authorized party I, {{ signer }}, wish to update the vote stake of account to the current value of my {{ REX }} balance.

<h1 class="contract">
   rentcpu
</h1>
---

spec-version: 0.0
title: Rent CPU
summary: The `rentcpu` action allows an account to rent CPU bandwidth for 30 days at a market-determined price.

icon: https://boscore.io/icon_256.png#b264b855c6d3335e5ee213443f679fb87c3633de8bc31cf66a766daac6dc6d7c
---

As an authorized party I, {{ signer }}, wish to rent CPU bandwidth for 30 days for the use of the account {{ receiver }} in exchange for the loan payment of {{ loan_payment }}, which shall be taken from the account {{ from }}. The loan fund amount {{ loan_fund }} is set for automatic renewal of the loan at the expiry of said loan.

The amount of CPU bandwidth shall be determined by the market at time of loan execution and shall be recalculated at time of renewal, should I wish to automatically renew the loan at that time. I acknowledge that the amount of CPU bandwidth received in exchange of {{ loan_payment }} for the benefit of {{ receiver }} at loan renewal may be different from the current amount of bandwidth received.

<h1 class="contract">
   rentnet
</h1>
---

spec-version: 0.0
title: Rent NET
summary: The `rentnet` action allows an account to rent Network bandwidth for 30 days at a market-determined price.

icon: https://boscore.io/icon_256.png#b264b855c6d3335e5ee213443f679fb87c3633de8bc31cf66a766daac6dc6d7c
---

As an authorized party I, {{ signer }}, wish to rent Network bandwidth for 30 days for the use of the account {{ receiver }} in exchange for the loan payment of {{ loan_payment }}, which shall be taken from the account {{ from }}. The loan fund amount {{ loan_fund }} is set for automatic renewal of the loan at the expiry of said loan.

The amount of Network bandwidth shall be determined by the market at time of loan execution and shall be recalculated at time of renewal, should I wish to automatically renew the loan at that time. I acknowledge that the amount of Network bandwidth received in exchange of {{ loan_payment }} for the benefit of {{ receiver }} at loan renewal may be different from the current amount of bandwidth received.

<h1 class="contract">
   rexexec
</h1>
---

spec-version: 0.0
title: REX Exec
summary: The `rexexec` action allows any account to perform REX maintenance by processing expired loans and unfilled sell orders.

icon: https://boscore.io/icon_256.png#b264b855c6d3335e5ee213443f679fb87c3633de8bc31cf66a766daac6dc6d7c
---

I, {{ signer }}, wish to process up to {{ max }} of any CPU loans, Network loans, and sell orders that may currently be pending.

<h1 class="contract">
   sellrex
</h1>
---

spec-version: 0.0
title: Sell REX
summary: The `sellrex` action allows an account to sell REX tokens held by the account.

icon: https://boscore.io/icon_256.png#b264b855c6d3335e5ee213443f679fb87c3633de8bc31cf66a766daac6dc6d7c
---

As an authorized party I, {{ signer }}, wish to sell {{ rex }} REX tokens held on the account {{ from }} in exchange for core EOS tokens. If there is an insufficient amount of EOS tokens available at this time, I acknowledge that my order will be placed in a queue to be processed.

If there is an open `sellrex` order for the account {{ from }}, then this amount of {{ rex }} REX shall be added to the existing order and the order shall move to the back of the queue.

<h1 class="contract">
   unstaketorex
</h1>
---

spec-version: 0.0
title: Unstake to REX
summary: The `unstaketorex` action allows an account to buy REX using EOS tokens which are currently staked for either CPU or Network bandwidth.

icon: https://boscore.io/icon_256.png#b264b855c6d3335e5ee213443f679fb87c3633de8bc31cf66a766daac6dc6d7c
---

As an authorized party I, {{ signer }}, wish to buy REX tokens by unstaking {{ from_cpu }} EOS from CPU bandwidth and {{ from_net }} EOS from Network bandwidth from account {{ owner }} that are staked to account {{ receiver }}.

I am aware of, and have fulfilled, all voting requirements needed to participate in the REX marketplace.

<h1 class="contract">
   withdraw
</h1>
---

spec-version: 0.0
title: withdraw from REX
summary: The `withdraw` action allows an account to withdraw EOS tokens from their REX fund into their liquid token balance.


icon: https://boscore.io/icon_256.png#b264b855c6d3335e5ee213443f679fb87c3633de8bc31cf66a766daac6dc6d7c
---

As an authorized party I, {{ signer }}, wish to withdraw {{ amount }} of EOS tokens from the REX fund for the account {{ owner }} into its liquid token balance.