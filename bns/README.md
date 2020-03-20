# bns - Blockchain Network Simulator

## Contact
For further questions, please contact `elias.rohrer AT tu-berlin.de`.

## Usage 
To be used in conjunction with the [ns-3 network simulator][ns3].

    # Extract and build ns-3.
    # Then, in ns-3.xx:

    ln -s scratch/bns /path/to/bns
    ./waf --run "bns --help"


[ns3]: https://www.nsnam.org

## Citation
The bns simulator was first published in 
> Rohrer, Elias, and Florian Tschorsch. "Kadcast: A Structured Approach to Broadcast in Blockchain Networks." Proceedings of the 1st ACM Conference on Advances in Financial Technologies. ACM, 2019. [Link](https://dl.acm.org/citation.cfm?id=3355469).

Please cite as:

    @inproceedings{rohrer2019kadcast,
      title={Kadcast: A Structured Approach to Broadcast in Blockchain Networks},
      author={Rohrer, Elias and Tschorsch, Florian},
      booktitle={AFT~'19: Proceedings of the 1st ACM Conference on Advances in Financial Technologies},
      pages={199--213},
      year={2019},
      organization={ACM}
    }
